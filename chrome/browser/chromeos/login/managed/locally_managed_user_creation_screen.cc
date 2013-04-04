// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/managed/locally_managed_user_creation_screen.h"

#include "chrome/browser/chromeos/login/error_screen.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/managed/locally_managed_user_controller.h"
#include "chrome/browser/chromeos/login/screen_observer.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace chromeos {

namespace {

void ConfigureErrorScreen(ErrorScreen* screen,
    const Network* network,
    const NetworkPortalDetector::CaptivePortalStatus status) {
  switch (status) {
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_UNKNOWN:
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE:
      NOTREACHED();
      break;
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_OFFLINE:
      screen->SetErrorState(ErrorScreen::ERROR_STATE_OFFLINE,
                            std::string());
      break;
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PORTAL:
      screen->SetErrorState(ErrorScreen::ERROR_STATE_PORTAL,
                            network->name());
      screen->FixCaptivePortal();
      break;
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PROXY_AUTH_REQUIRED:
      screen->SetErrorState(ErrorScreen::ERROR_STATE_PROXY,
                            std::string());
      break;
    default:
      NOTREACHED();
      break;
  }
}

} // namespace

LocallyManagedUserCreationScreen::LocallyManagedUserCreationScreen(
    ScreenObserver* observer,
    LocallyManagedUserCreationScreenHandler* actor)
    : WizardScreen(observer),
      actor_(actor),
      on_error_screen_(false) {
  DCHECK(actor_);
  if (actor_)
    actor_->SetDelegate(this);
}

LocallyManagedUserCreationScreen::~LocallyManagedUserCreationScreen() {
  if (actor_)
    actor_->SetDelegate(NULL);
}

void LocallyManagedUserCreationScreen::PrepareToShow() {
  if (actor_)
    actor_->PrepareToShow();
}

void LocallyManagedUserCreationScreen::Show() {
  if (actor_) {
    actor_->Show();
    actor_->ShowInitialScreen();
  }

  NetworkPortalDetector* detector = NetworkPortalDetector::GetInstance();
  if (detector && !on_error_screen_) {
    detector->AddObserver(this);
    detector->ForcePortalDetection();
  }
  on_error_screen_ = false;
}

void LocallyManagedUserCreationScreen::OnPortalDetectionCompleted(
    const Network* network,
    const NetworkPortalDetector::CaptivePortalState& state)  {
  if (state.status == NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE) {
    get_screen_observer()->HideErrorScreen(this);
  } else {
    on_error_screen_ = true;
    ErrorScreen* screen = get_screen_observer()->GetErrorScreen();
    ConfigureErrorScreen(screen, network, state.status);
    screen->SetUIState(ErrorScreen::UI_STATE_LOCALLY_MANAGED);
    get_screen_observer()->ShowErrorScreen();
  }
}

void LocallyManagedUserCreationScreen::
    ShowManagerInconsistentStateErrorScreen() {
  if (!actor_)
    return;
  actor_->ShowErrorMessage(
      l10n_util::GetStringUTF16(
          IDS_CREATE_LOCALLY_MANAGED_USER_MANAGER_INCONSISTENT_STATE),
      false);
}

void LocallyManagedUserCreationScreen::ShowInitialScreen() {
  if (actor_)
    actor_->ShowInitialScreen();
}

void LocallyManagedUserCreationScreen::Hide() {
  if (actor_)
    actor_->Hide();
  NetworkPortalDetector* detector = NetworkPortalDetector::GetInstance();
  if (detector && !on_error_screen_)
    detector->RemoveObserver(this);
}

std::string LocallyManagedUserCreationScreen::GetName() const {
  return WizardController::kLocallyManagedUserCreationScreenName;
}

void LocallyManagedUserCreationScreen::AbortFlow() {
  controller_->FinishCreation();
}

void LocallyManagedUserCreationScreen::FinishFlow() {
  controller_->FinishCreation();
}

void LocallyManagedUserCreationScreen::RetryLastStep() {
  controller_->RetryLastStep();
}

void LocallyManagedUserCreationScreen::RunFlow(
    string16& display_name,
    std::string& managed_user_password,
    std::string& manager_id,
    std::string& manager_password) {

  // Make sure no two controllers exist at the same time.
  controller_.reset();
  controller_.reset(new LocallyManagedUserController(this));
  controller_->SetUpCreation(display_name, managed_user_password);

  ExistingUserController::current_controller()->
      Login(UserContext(manager_id,
                        manager_password,
                        std::string()  /* auth_code */));
}

void LocallyManagedUserCreationScreen::OnManagerLoginFailure() {
  if (actor_)
    actor_->ShowManagerPasswordError();
}

void LocallyManagedUserCreationScreen::OnManagerSignIn() {
  if (actor_)
    actor_->ShowProgressScreen();
  controller_->StartCreation();
}

void LocallyManagedUserCreationScreen::OnExit() {}

void LocallyManagedUserCreationScreen::OnActorDestroyed(
    LocallyManagedUserCreationScreenHandler* actor) {
  if (actor_ == actor)
    actor_ = NULL;
}

void LocallyManagedUserCreationScreen::OnCreationError(
    LocallyManagedUserController::ErrorCode code,
    bool recoverable) {
  string16 message;
  // TODO(antrim) : find out which errors do we really have.
  // We might reuse some error messages from ordinary user flow.
  switch (code) {
    case LocallyManagedUserController::CRYPTOHOME_NO_MOUNT:
    case LocallyManagedUserController::CRYPTOHOME_FAILED_MOUNT:
    case LocallyManagedUserController::CRYPTOHOME_FAILED_TPM:
      message = l10n_util::GetStringUTF16(
          IDS_CREATE_LOCALLY_MANAGED_USER_CREATION_ERROR_TPM_ERROR);
      break;
    case LocallyManagedUserController::CLOUD_NOT_CONNECTED:
      message = l10n_util::GetStringUTF16(
          IDS_CREATE_LOCALLY_MANAGED_USER_CREATION_ERROR_NOT_CONNECTED);
      break;
    case LocallyManagedUserController::CLOUD_TIMED_OUT:
      message = l10n_util::GetStringUTF16(
          IDS_CREATE_LOCALLY_MANAGED_USER_CREATION_ERROR_TIMED_OUT);
      break;
    case LocallyManagedUserController::CLOUD_SERVER_ERROR:
      message = l10n_util::GetStringUTF16(
          IDS_CREATE_LOCALLY_MANAGED_USER_CREATION_ERROR_SERVER_ERROR);
      break;
    default:
      NOTREACHED();
  }
  if (actor_)
    actor_->ShowErrorMessage(message, recoverable);
}

void LocallyManagedUserCreationScreen::OnCreationSuccess() {
  if (actor_)
    actor_->ShowSuccessMessage();
}

}  // namespace chromeos
