// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/app_mode/kiosk_profile_loader.h"

#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager.h"
#include "chrome/browser/chromeos/login/auth/login_status_consumer.h"
#include "chrome/browser/chromeos/login/auth/user_context.h"
#include "chrome/browser/chromeos/login/demo_mode/demo_app_launcher.h"
#include "chrome/browser/chromeos/login/login_utils.h"
#include "chrome/browser/chromeos/login/ui/login_display_host_impl.h"
#include "chrome/browser/chromeos/login/users/user_manager.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chromeos/cryptohome/async_method_caller.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/gaia/gaia_auth_util.h"

using content::BrowserThread;

namespace chromeos {

namespace {

KioskAppLaunchError::Error LoginFailureToKioskAppLaunchError(
    const LoginFailure& error) {
  switch (error.reason()) {
    case LoginFailure::COULD_NOT_MOUNT_TMPFS:
    case LoginFailure::COULD_NOT_MOUNT_CRYPTOHOME:
      return KioskAppLaunchError::UNABLE_TO_MOUNT;
    case LoginFailure::DATA_REMOVAL_FAILED:
      return KioskAppLaunchError::UNABLE_TO_REMOVE;
    case LoginFailure::USERNAME_HASH_FAILED:
      return KioskAppLaunchError::UNABLE_TO_RETRIEVE_HASH;
    default:
      NOTREACHED();
      return KioskAppLaunchError::UNABLE_TO_MOUNT;
  }
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// KioskProfileLoader::CryptohomedChecker ensures cryptohome daemon is up
// and running by issuing an IsMounted call. If the call does not go through
// and chromeos::DBUS_METHOD_CALL_SUCCESS is not returned, it will retry after
// some time out and at the maximum five times before it gives up. Upon
// success, it resumes the launch by logging in as a kiosk mode account.

class KioskProfileLoader::CryptohomedChecker
    : public base::SupportsWeakPtr<CryptohomedChecker> {
 public:
  explicit CryptohomedChecker(KioskProfileLoader* loader)
      : loader_(loader),
        retry_count_(0) {
  }
  ~CryptohomedChecker() {}

  void StartCheck() {
    chromeos::DBusThreadManager::Get()->GetCryptohomeClient()->IsMounted(
        base::Bind(&CryptohomedChecker::OnCryptohomeIsMounted,
                   AsWeakPtr()));
  }

 private:
  void OnCryptohomeIsMounted(chromeos::DBusMethodCallStatus call_status,
                             bool is_mounted) {
    if (call_status != chromeos::DBUS_METHOD_CALL_SUCCESS) {
      const int kMaxRetryTimes = 5;
      ++retry_count_;
      if (retry_count_ > kMaxRetryTimes) {
        LOG(ERROR) << "Could not talk to cryptohomed for launching kiosk app.";
        ReportCheckResult(KioskAppLaunchError::CRYPTOHOMED_NOT_RUNNING);
        return;
      }

      const int retry_delay_in_milliseconds = 500 * (1 << retry_count_);
      base::MessageLoop::current()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&CryptohomedChecker::StartCheck, AsWeakPtr()),
          base::TimeDelta::FromMilliseconds(retry_delay_in_milliseconds));
      return;
    }

    if (is_mounted)
      LOG(ERROR) << "Cryptohome is mounted before launching kiosk app.";

    // Proceed only when cryptohome is not mounded or running on dev box.
    if (!is_mounted || !base::SysInfo::IsRunningOnChromeOS())
      ReportCheckResult(KioskAppLaunchError::NONE);
    else
      ReportCheckResult(KioskAppLaunchError::ALREADY_MOUNTED);
  }

  void ReportCheckResult(KioskAppLaunchError::Error error) {
    if (error == KioskAppLaunchError::NONE)
      loader_->LoginAsKioskAccount();
    else
      loader_->ReportLaunchResult(error);
  }

  KioskProfileLoader* loader_;
  int retry_count_;

  DISALLOW_COPY_AND_ASSIGN(CryptohomedChecker);
};


////////////////////////////////////////////////////////////////////////////////
// KioskProfileLoader

KioskProfileLoader::KioskProfileLoader(const std::string& app_user_id,
                                       bool use_guest_mount,
                                       Delegate* delegate)
    : user_id_(app_user_id),
      use_guest_mount_(use_guest_mount),
      delegate_(delegate) {}

KioskProfileLoader::~KioskProfileLoader() {}

void KioskProfileLoader::Start() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  login_performer_.reset();
  cryptohomed_checker_.reset(new CryptohomedChecker(this));
  cryptohomed_checker_->StartCheck();
}

void KioskProfileLoader::LoginAsKioskAccount() {
  login_performer_.reset(new LoginPerformer(this));
  login_performer_->LoginAsKioskAccount(user_id_, use_guest_mount_);
}

void KioskProfileLoader::ReportLaunchResult(KioskAppLaunchError::Error error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (error != KioskAppLaunchError::NONE) {
    delegate_->OnProfileLoadFailed(error);
  }
}

void KioskProfileLoader::OnLoginSuccess(const UserContext& user_context)  {
  // LoginPerformer will delete itself.
  login_performer_->set_delegate(NULL);
  ignore_result(login_performer_.release());

  // If we are launching a demo session, we need to start MountGuest with the
  // guest username; this is because there are several places in the cros code
  // which rely on the username sent to cryptohome to be $guest. Back in Chrome
  // we switch this back to the demo user name to correctly identify this
  // user as a demo user.
  UserContext context = user_context;
  if (context.GetUserID() == UserManager::kGuestUserName)
    context.SetUserID(DemoAppLauncher::kDemoUserName);
  LoginUtils::Get()->PrepareProfile(context,
                                    std::string(),  // display email
                                    false,  // has_cookies
                                    false,  // has_active_session
                                    this);
}

void KioskProfileLoader::OnLoginFailure(const LoginFailure& error) {
  ReportLaunchResult(LoginFailureToKioskAppLaunchError(error));
}

void KioskProfileLoader::WhiteListCheckFailed(const std::string& email) {
  NOTREACHED();
}

void KioskProfileLoader::PolicyLoadFailed() {
  ReportLaunchResult(KioskAppLaunchError::POLICY_LOAD_FAILED);
}

void KioskProfileLoader::OnOnlineChecked(
    const std::string& email, bool success) {
  NOTREACHED();
}

void KioskProfileLoader::OnProfilePrepared(Profile* profile) {
  // This object could be deleted any time after successfully reporting
  // a profile load, so invalidate the LoginUtils delegate now.
  LoginUtils::Get()->DelegateDeleted(this);

  delegate_->OnProfileLoaded(profile);
  ReportLaunchResult(KioskAppLaunchError::NONE);
}

}  // namespace chromeos
