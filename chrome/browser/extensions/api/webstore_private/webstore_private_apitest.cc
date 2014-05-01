// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/callback_list.h"
#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/api/identity/identity_api.h"
#include "chrome/browser/extensions/api/management/management_api.h"
#include "chrome/browser/extensions/api/webstore_private/webstore_private_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "chrome/browser/extensions/extension_install_ui.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/webstore_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/fake_profile_oauth2_token_service.h"
#include "chrome/browser/signin/fake_profile_oauth2_token_service_builder.h"
#include "chrome/browser/signin/fake_signin_manager.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/test/browser_test_utils.h"
#include "gpu/config/gpu_feature_type.h"
#include "gpu/config/gpu_info.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/gl/gl_switches.h"

using gpu::GpuFeatureType;

namespace utils = extension_function_test_utils;

namespace extensions {

namespace {

class WebstoreInstallListener : public WebstoreInstaller::Delegate {
 public:
  WebstoreInstallListener()
      : received_failure_(false), received_success_(false), waiting_(false) {}

  virtual void OnExtensionInstallSuccess(const std::string& id) OVERRIDE {
    received_success_ = true;
    id_ = id;

    if (waiting_) {
      waiting_ = false;
      base::MessageLoopForUI::current()->Quit();
    }
  }

  virtual void OnExtensionInstallFailure(
      const std::string& id,
      const std::string& error,
      WebstoreInstaller::FailureReason reason) OVERRIDE {
    received_failure_ = true;
    id_ = id;
    error_ = error;

    if (waiting_) {
      waiting_ = false;
      base::MessageLoopForUI::current()->Quit();
    }
  }

  void Wait() {
    if (received_success_ || received_failure_)
      return;

    waiting_ = true;
    content::RunMessageLoop();
  }
  bool received_success() const { return received_success_; }
  const std::string& id() const { return id_; }

 private:
  bool received_failure_;
  bool received_success_;
  bool waiting_;
  std::string id_;
  std::string error_;
};

}  // namespace

// A base class for tests below.
class ExtensionWebstorePrivateApiTest : public ExtensionApiTest {
 public:
  ExtensionWebstorePrivateApiTest()
      : signin_manager_(NULL),
        token_service_(NULL) {}
  virtual ~ExtensionWebstorePrivateApiTest() {}

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        switches::kAppsGalleryURL,
        "http://www.example.com/files/extensions/api_test");
    command_line->AppendSwitchASCII(
        switches::kAppsGalleryInstallAutoConfirmForTests, "accept");
  }

  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    ExtensionApiTest::SetUpInProcessBrowserTestFixture();

    // Start up the test server and get us ready for calling the install
    // API functions.
    host_resolver()->AddRule("www.example.com", "127.0.0.1");
    ASSERT_TRUE(StartSpawnedTestServer());
    ExtensionInstallUI::set_disable_failure_ui_for_tests();

    will_create_browser_context_services_subscription_ =
        BrowserContextDependencyManager::GetInstance()->
            RegisterWillCreateBrowserContextServicesCallbackForTesting(
                base::Bind(
                    &ExtensionWebstorePrivateApiTest::
                        OnWillCreateBrowserContextServices,
                    base::Unretained(this))).Pass();
  }

  void OnWillCreateBrowserContextServices(content::BrowserContext* context) {
    // Replace the signin manager and token service with fakes. Do this ahead of
    // creating the browser so that a bunch of classes don't register as
    // observers and end up needing to unregister when the fake is substituted.
    SigninManagerFactory::GetInstance()->SetTestingFactory(
        context, &FakeSigninManagerBase::Build);
    ProfileOAuth2TokenServiceFactory::GetInstance()->SetTestingFactory(
        context, &BuildFakeProfileOAuth2TokenService);
  }

  virtual void SetUpOnMainThread() OVERRIDE {
    ExtensionApiTest::SetUpOnMainThread();

    // Grab references to the fake signin manager and token service.
    signin_manager_ =
        static_cast<FakeSigninManagerForTesting*>(
            SigninManagerFactory::GetInstance()->GetForProfile(profile()));
    ASSERT_TRUE(signin_manager_);
    token_service_ =
        static_cast<FakeProfileOAuth2TokenService*>(
            ProfileOAuth2TokenServiceFactory::GetInstance()->GetForProfile(
                profile()));
    ASSERT_TRUE(token_service_);
  }

 protected:
  // Returns a test server URL, but with host 'www.example.com' so it matches
  // the web store app's extent that we set up via command line flags.
  virtual GURL GetTestServerURL(const std::string& path) {
    GURL url = test_server()->GetURL(
        std::string("files/extensions/api_test/webstore_private/") + path);

    // Replace the host with 'www.example.com' so it matches the web store
    // app's extent.
    GURL::Replacements replace_host;
    std::string host_str("www.example.com");
    replace_host.SetHostStr(host_str);

    return url.ReplaceComponents(replace_host);
  }

  // Navigates to |page| and runs the Extension API test there. Any downloads
  // of extensions will return the contents of |crx_file|.
  bool RunInstallTest(const std::string& page, const std::string& crx_file) {
    // Auto-confirm the uninstallation dialog.
    ManagementUninstallFunction::SetAutoConfirmForTest(true);
#if defined(OS_WIN) && !defined(NDEBUG)
    // See http://crbug.com/177163 for details.
    return true;
#else
    GURL crx_url = GetTestServerURL(crx_file);
    CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kAppsGalleryUpdateURL, crx_url.spec());

    GURL page_url = GetTestServerURL(page);
    return RunPageTest(page_url.spec());
#endif
  }

  // Navigates to |page| and waits for the API call.
  void StartSignInTest(const std::string& page) {
    ui_test_utils::NavigateToURL(browser(), GetTestServerURL(page));

    // Wait for the API to be called.  A simple way to wait for this is to run
    // some other JavaScript in the page and wait for a round-trip back to the
    // browser process.
    bool result = false;
    ASSERT_TRUE(
        content::ExecuteScriptAndExtractBool(
            GetWebContents(), "window.domAutomationController.send(true)",
            &result));
    ASSERT_TRUE(result);
  }

  content::WebContents* GetWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  ExtensionService* service() {
    return browser()->profile()->GetExtensionService();
  }

  FakeSigninManagerForTesting* signin_manager_;
  FakeProfileOAuth2TokenService* token_service_;

 private:
  scoped_ptr<base::CallbackList<void(content::BrowserContext*)>::Subscription>
      will_create_browser_context_services_subscription_;

};

// Test cases for webstore origin frame blocking.
// TODO(mkwst): Disabled until new X-Frame-Options behavior rolls into
// Chromium, see crbug.com/226018.
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       DISABLED_FrameWebstorePageBlocked) {
  base::string16 expected_title = base::UTF8ToUTF16("PASS: about:blank");
  base::string16 failure_title = base::UTF8ToUTF16("FAIL");
  content::TitleWatcher watcher(GetWebContents(), expected_title);
  watcher.AlsoWaitForTitle(failure_title);
  GURL url = test_server()->GetURL(
      "files/extensions/api_test/webstore_private/noframe.html");
  ui_test_utils::NavigateToURL(browser(), url);
  base::string16 final_title = watcher.WaitAndGetTitle();
  EXPECT_EQ(expected_title, final_title);
}

// TODO(mkwst): Disabled until new X-Frame-Options behavior rolls into
// Chromium, see crbug.com/226018.
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       DISABLED_FrameErrorPageBlocked) {
  base::string16 expected_title = base::UTF8ToUTF16("PASS: about:blank");
  base::string16 failure_title = base::UTF8ToUTF16("FAIL");
  content::TitleWatcher watcher(GetWebContents(), expected_title);
  watcher.AlsoWaitForTitle(failure_title);
  GURL url = test_server()->GetURL(
      "files/extensions/api_test/webstore_private/noframe2.html");
  ui_test_utils::NavigateToURL(browser(), url);
  base::string16 final_title = watcher.WaitAndGetTitle();
  EXPECT_EQ(expected_title, final_title);
}

// Test cases where the user accepts the install confirmation dialog.
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, InstallAccepted) {
  ASSERT_TRUE(RunInstallTest("accepted.html", "extension.crx"));
}

// Test having the default download directory missing.
 IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, MissingDownloadDir) {
  // Set a non-existent directory as the download path.
  base::ScopedTempDir temp_dir;
  EXPECT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath missing_directory = temp_dir.Take();
  EXPECT_TRUE(base::DeleteFile(missing_directory, true));
  WebstoreInstaller::SetDownloadDirectoryForTests(&missing_directory);

  // Now run the install test, which should succeed.
  ASSERT_TRUE(RunInstallTest("accepted.html", "extension.crx"));

  // Cleanup.
  if (base::DirectoryExists(missing_directory))
    EXPECT_TRUE(base::DeleteFile(missing_directory, true));
}

// Tests passing a localized name.
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, InstallLocalized) {
  ASSERT_TRUE(RunInstallTest("localized.html", "localized_extension.crx"));
}

// Now test the case where the user cancels the confirmation dialog.
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, InstallCancelled) {
  CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "cancel");
  ASSERT_TRUE(RunInstallTest("cancelled.html", "extension.crx"));
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, IncorrectManifest1) {
  ASSERT_TRUE(RunInstallTest("incorrect_manifest1.html", "extension.crx"));
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, IncorrectManifest2) {
  ASSERT_TRUE(RunInstallTest("incorrect_manifest2.html", "extension.crx"));
}

// Disabled: http://crbug.com/174399 and http://crbug.com/177163
#if defined(OS_WIN) && (defined(USE_AURA) || !defined(NDEBUG))
#define MAYBE_AppInstallBubble DISABLED_AppInstallBubble
#else
#define MAYBE_AppInstallBubble AppInstallBubble
#endif

// Tests that we can request an app installed bubble (instead of the default
// UI when an app is installed).
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       MAYBE_AppInstallBubble) {
  WebstoreInstallListener listener;
  WebstorePrivateApi::SetWebstoreInstallerDelegateForTesting(&listener);
  ASSERT_TRUE(RunInstallTest("app_install_bubble.html", "app.crx"));
  listener.Wait();
  ASSERT_TRUE(listener.received_success());
  ASSERT_EQ("iladmdjkfniedhfhcfoefgojhgaiaccc", listener.id());
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, IsInIncognitoMode) {
  GURL page_url = GetTestServerURL("incognito.html");
  ASSERT_TRUE(
      RunPageTest(page_url.spec(), ExtensionApiTest::kFlagUseIncognito));
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, IsNotInIncognitoMode) {
  GURL page_url = GetTestServerURL("not_incognito.html");
  ASSERT_TRUE(RunPageTest(page_url.spec()));
}

// Fails often on Windows dbg bots. http://crbug.com/177163.
#if defined(OS_WIN)
#define MAYBE_IconUrl DISABLED_IconUrl
#else
#define MAYBE_IconUrl IconUrl
#endif  // defined(OS_WIN)
// Tests using the iconUrl parameter to the install function.
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, MAYBE_IconUrl) {
  ASSERT_TRUE(RunInstallTest("icon_url.html", "extension.crx"));
}

// http://crbug.com/177163
#if defined(OS_WIN) && !defined(NDEBUG)
#define MAYBE_BeginInstall DISABLED_BeginInstall
#else
#define MAYBE_BeginInstall BeginInstall
#endif
// Tests that the Approvals are properly created in beginInstall.
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, MAYBE_BeginInstall) {
  std::string appId = "iladmdjkfniedhfhcfoefgojhgaiaccc";
  std::string extensionId = "enfkhcelefdadlmkffamgdlgplcionje";
  ASSERT_TRUE(RunInstallTest("begin_install.html", "extension.crx"));

  scoped_ptr<WebstoreInstaller::Approval> approval =
      WebstorePrivateApi::PopApprovalForTesting(browser()->profile(), appId);
  EXPECT_EQ(appId, approval->extension_id);
  EXPECT_TRUE(approval->use_app_installed_bubble);
  EXPECT_FALSE(approval->skip_post_install_ui);
  EXPECT_EQ(browser()->profile(), approval->profile);

  approval = WebstorePrivateApi::PopApprovalForTesting(
      browser()->profile(), extensionId);
  EXPECT_EQ(extensionId, approval->extension_id);
  EXPECT_FALSE(approval->use_app_installed_bubble);
  EXPECT_FALSE(approval->skip_post_install_ui);
  EXPECT_EQ(browser()->profile(), approval->profile);
}

// http://crbug.com/177163
#if defined(OS_WIN) && !defined(NDEBUG)
#define MAYBE_InstallTheme DISABLED_InstallTheme
#else
#define MAYBE_InstallTheme InstallTheme
#endif
// Tests that themes are installed without an install prompt.
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, MAYBE_InstallTheme) {
  WebstoreInstallListener listener;
  WebstorePrivateApi::SetWebstoreInstallerDelegateForTesting(&listener);
  ASSERT_TRUE(RunInstallTest("theme.html", "../../theme.crx"));
  listener.Wait();
  ASSERT_TRUE(listener.received_success());
  ASSERT_EQ("iamefpfkojoapidjnbafmgkgncegbkad", listener.id());
}

// Tests that an error is properly reported when an empty crx is returned.
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest, EmptyCrx) {
  ASSERT_TRUE(RunInstallTest("empty.html", "empty.crx"));
}

class ExtensionWebstoreGetWebGLStatusTest : public InProcessBrowserTest {
 protected:
  void RunTest(bool webgl_allowed) {
    // If Gpu access is disallowed then WebGL will not be available.
    if (!content::GpuDataManager::GetInstance()->GpuAccessAllowed(NULL))
      webgl_allowed = false;

    static const char kEmptyArgs[] = "[]";
    static const char kWebGLStatusAllowed[] = "webgl_allowed";
    static const char kWebGLStatusBlocked[] = "webgl_blocked";
    scoped_refptr<WebstorePrivateGetWebGLStatusFunction> function =
        new WebstorePrivateGetWebGLStatusFunction();
    scoped_ptr<base::Value> result(utils::RunFunctionAndReturnSingleResult(
            function.get(), kEmptyArgs, browser()));
    ASSERT_TRUE(result);
    EXPECT_EQ(base::Value::TYPE_STRING, result->GetType());
    std::string webgl_status;
    EXPECT_TRUE(result->GetAsString(&webgl_status));
    EXPECT_STREQ(webgl_allowed ? kWebGLStatusAllowed : kWebGLStatusBlocked,
                 webgl_status.c_str());
  }
};

// Tests getWebGLStatus function when WebGL is allowed.
IN_PROC_BROWSER_TEST_F(ExtensionWebstoreGetWebGLStatusTest, Allowed) {
  bool webgl_allowed = true;
  RunTest(webgl_allowed);
}

// Tests getWebGLStatus function when WebGL is blacklisted.
IN_PROC_BROWSER_TEST_F(ExtensionWebstoreGetWebGLStatusTest, Blocked) {
  static const std::string json_blacklist =
      "{\n"
      "  \"name\": \"gpu blacklist\",\n"
      "  \"version\": \"1.0\",\n"
      "  \"entries\": [\n"
      "    {\n"
      "      \"id\": 1,\n"
      "      \"features\": [\n"
      "        \"webgl\"\n"
      "      ]\n"
      "    }\n"
      "  ]\n"
      "}";
  gpu::GPUInfo gpu_info;
  content::GpuDataManager::GetInstance()->InitializeForTesting(
      json_blacklist, gpu_info);
  EXPECT_TRUE(content::GpuDataManager::GetInstance()->IsFeatureBlacklisted(
      gpu::GPU_FEATURE_TYPE_WEBGL));

  bool webgl_allowed = false;
  RunTest(webgl_allowed);
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_UserGestureRequired) {
  GURL page_url = GetTestServerURL("sign_in_user_gesture_required.html");
  ASSERT_TRUE(RunPageTest(page_url.spec()));
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_MissingContinueUrl) {
  GURL page_url = GetTestServerURL("sign_in_missing_continue_url.html");
  ASSERT_TRUE(RunPageTest(page_url.spec()));
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_InvalidContinueUrl) {
  GURL page_url = GetTestServerURL("sign_in_invalid_continue_url.html");
  ASSERT_TRUE(RunPageTest(page_url.spec()));
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_ContinueUrlOnDifferentOrigin) {
  GURL page_url =
      GetTestServerURL("sign_in_continue_url_on_different_origin.html");
  ASSERT_TRUE(RunPageTest(page_url.spec()));
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_DisallowedInIncognito) {
  // Make sure that the test is testing something more than the absence of a
  // sign-in manager for this profile.
  ASSERT_TRUE(SigninManagerFactory::GetForProfile(profile()));

  GURL page_url =
      GetTestServerURL("sign_in_disallowed_in_incognito.html");
  ASSERT_TRUE(
      RunPageTest(page_url.spec(), ExtensionApiTest::kFlagUseIncognito));
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_DisabledWhenWebBasedSigninIsEnabled) {
  // Make sure that the test is testing something more than the absence of a
  // sign-in manager for this profile.
  ASSERT_TRUE(SigninManagerFactory::GetForProfile(profile()));

  CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableWebBasedSignin);
  GURL page_url = GetTestServerURL(
      "sign_in_disabled_when_web_based_signin_is_enabled.html");
  ASSERT_TRUE(RunPageTest(page_url.spec()));
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_AlreadySignedIn) {
  signin_manager_->SetAuthenticatedUsername("user@example.com");
  GURL page_url = GetTestServerURL("sign_in_already_signed_in.html");
  ASSERT_TRUE(RunPageTest(page_url.spec()));
}

// The FakeSignInManager class is not implemented for ChromeOS, so there's no
// straightforward way to test these flows on that platform.
#if !defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_AuthInProgress_Fails) {
  // Initiate an authentication that will be in progress when the sign-in API is
  // called.
  signin_manager_->set_auth_in_progress("user@example.com");

  // Navigate to the page, which will cause the sign-in API to be called.
  // Then, complete the authentication in a failed state.
  ResultCatcher catcher;
  StartSignInTest("sign_in_auth_in_progress_fails.html");
  signin_manager_->FailSignin(GoogleServiceAuthError::AuthErrorNone());
  ASSERT_TRUE(catcher.GetNextResult());
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_AuthInProgress_MergeSessionFails) {
  // Initiate an authentication that will be in progress when the sign-in API is
  // called.
  signin_manager_->set_auth_in_progress("user@example.com");

  // Navigate to the page, which will cause the sign-in API to be called.
  // Then, complete the authentication in a successful state.
  ResultCatcher catcher;
  StartSignInTest("sign_in_auth_in_progress_merge_session_fails.html");
  signin_manager_->CompletePendingSignin();
  token_service_->IssueRefreshTokenForUser("user@example.com", "token");
  signin_manager_->NotifyMergeSessionObservers(
      GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE));
  ASSERT_TRUE(catcher.GetNextResult());
}

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_AuthInProgress_Succeeds) {
  // Initiate an authentication that will be in progress when the sign-in API is
  // called.
  signin_manager_->set_auth_in_progress("user@example.com");

  // Navigate to the page, which will cause the sign-in API to be called.
  // Then, complete the authentication in a successful state.
  ResultCatcher catcher;
  StartSignInTest("sign_in_auth_in_progress_succeeds.html");
  signin_manager_->CompletePendingSignin();
  token_service_->IssueRefreshTokenForUser("user@example.com", "token");
  signin_manager_->NotifyMergeSessionObservers(
      GoogleServiceAuthError::AuthErrorNone());
  ASSERT_TRUE(catcher.GetNextResult());
}
#endif  // !defined (OS_CHROMEOS)

IN_PROC_BROWSER_TEST_F(ExtensionWebstorePrivateApiTest,
                       SignIn_RedirectToSignIn) {
  GURL signin_url(
      "chrome://chrome-signin/?source=5&"
      "continue=http%3A%2F%2Fwww.example.com%3A" +
      base::IntToString(test_server()->host_port_pair().port()) +
      "%2Fcontinue");
  ui_test_utils::UrlLoadObserver observer(
      signin_url,
      content::Source<content::NavigationController>(
          &GetWebContents()->GetController()));
  StartSignInTest("sign_in_redirect_to_sign_in.html");
  observer.Wait();

  // TODO(isherman): Also test the redirect back to the continue URL once
  // sign-in completes?
}

}  // namespace extensions
