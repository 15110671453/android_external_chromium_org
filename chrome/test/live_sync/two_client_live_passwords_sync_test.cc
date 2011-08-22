// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/utf_string_conversions.h"
#include "chrome/browser/sync/engine/model_safe_worker.h"
#include "chrome/browser/sync/profile_sync_service_harness.h"
#include "chrome/browser/sync/sessions/session_state.h"
#include "chrome/test/live_sync/live_passwords_sync_test.h"

using webkit_glue::PasswordForm;

static const char* kValidPassphrase = "passphrase!";

// TCM ID - 3732277
// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, DISABLED_Add) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, Add) {
#endif
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());

  PasswordForm form = CreateTestPasswordForm(0);
  AddLogin(GetVerifierPasswordStore(), form);
  ASSERT_EQ(1, GetVerifierPasswordCount());
  AddLogin(GetPasswordStore(0), form);
  ASSERT_EQ(1, GetPasswordCount(0));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());
}

// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, DISABLED_Race) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, Race) {
#endif
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AllProfilesContainSamePasswordForms());

  PasswordForm form0 = CreateTestPasswordForm(0);
  AddLogin(GetPasswordStore(0), form0);

  PasswordForm form1 = form0;
  form1.password_value = ASCIIToUTF16("password1");
  AddLogin(GetPasswordStore(1), form1);

  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AllProfilesContainSamePasswordForms());
}

// TCM ID - 4577932.
// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest,
                       DISABLED_DisablePasswords) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, DisablePasswords) {
#endif
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());

  ASSERT_TRUE(GetClient(1)->DisableSyncForDatatype(syncable::PASSWORDS));
  PasswordForm form = CreateTestPasswordForm(0);
  AddLogin(GetVerifierPasswordStore(), form);
  ASSERT_EQ(1, GetVerifierPasswordCount());
  AddLogin(GetPasswordStore(0), form);
  ASSERT_EQ(1, GetPasswordCount(0));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(ProfileContainsSamePasswordFormsAsVerifier(0));
  ASSERT_FALSE(ProfileContainsSamePasswordFormsAsVerifier(1));

  ASSERT_TRUE(GetClient(1)->EnableSyncForDatatype(syncable::PASSWORDS));
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());
  ASSERT_EQ(1, GetPasswordCount(1));
}

// TCM ID - 4649281.
// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, DISABLED_DisableSync) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, DisableSync) {
#endif
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());

  ASSERT_TRUE(GetClient(1)->DisableSyncForAllDatatypes());
  PasswordForm form = CreateTestPasswordForm(0);
  AddLogin(GetVerifierPasswordStore(), form);
  ASSERT_EQ(1, GetVerifierPasswordCount());
  AddLogin(GetPasswordStore(0), form);
  ASSERT_EQ(1, GetPasswordCount(0));

  ASSERT_TRUE(GetClient(0)->AwaitSyncCycleCompletion("Added a password."));
  ASSERT_TRUE(ProfileContainsSamePasswordFormsAsVerifier(0));
  ASSERT_FALSE(ProfileContainsSamePasswordFormsAsVerifier(1));

  ASSERT_TRUE(GetClient(1)->EnableSyncForAllDatatypes());
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());
  ASSERT_EQ(1, GetPasswordCount(1));
}

// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest,
                       DISABLED_SetPassphrase) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, SetPassphrase) {
#endif
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  SetPassphrase(0, kValidPassphrase, true);
  ASSERT_TRUE(GetClient(0)->AwaitPassphraseAccepted());
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  SetPassphrase(1, kValidPassphrase, false);
  ASSERT_TRUE(GetClient(1)->AwaitPassphraseAccepted());
  ASSERT_TRUE(GetClient(1)->AwaitSyncCycleCompletion("Set passphrase."));
}

// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest,
                       DISABLED_SetPassphraseAndAddPassword) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest,
                       SetPassphraseAndAddPassword) {
#endif
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  SetPassphrase(0, kValidPassphrase, true);
  ASSERT_TRUE(GetClient(0)->AwaitPassphraseAccepted());
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  SetPassphrase(1, kValidPassphrase, false);
  ASSERT_TRUE(GetClient(1)->AwaitPassphraseAccepted());

  PasswordForm form = CreateTestPasswordForm(0);
  AddLogin(GetPasswordStore(0), form);
  ASSERT_EQ(1, GetPasswordCount(0));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_EQ(1, GetPasswordCount(1));
}

// TCM ID - 4603879
// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, DISABLED_Update) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, Update) {
#endif
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());

  PasswordForm form = CreateTestPasswordForm(0);
  AddLogin(GetVerifierPasswordStore(), form);
  AddLogin(GetPasswordStore(0), form);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  form.password_value = ASCIIToUTF16("updated");
  UpdateLogin(GetVerifierPasswordStore(), form);
  UpdateLogin(GetPasswordStore(1), form);
  ASSERT_TRUE(AwaitQuiescence());

  ASSERT_EQ(1, GetVerifierPasswordCount());
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());
}

// TCM ID - 3719309
// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, DISABLED_Delete) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, Delete) {
#endif
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());

  PasswordForm form0 = CreateTestPasswordForm(0);
  AddLogin(GetVerifierPasswordStore(), form0);
  AddLogin(GetPasswordStore(0), form0);
  PasswordForm form1 = CreateTestPasswordForm(1);
  AddLogin(GetVerifierPasswordStore(), form1);
  AddLogin(GetPasswordStore(0), form1);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  RemoveLogin(GetPasswordStore(1), form0);
  RemoveLogin(GetVerifierPasswordStore(), form0);
  ASSERT_TRUE(AwaitQuiescence());

  ASSERT_EQ(1, GetVerifierPasswordCount());
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());
}

// TCM ID - 7573511
// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, DISABLED_DeleteAll) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, DeleteAll) {
#endif
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());

  PasswordForm form0 = CreateTestPasswordForm(0);
  AddLogin(GetVerifierPasswordStore(), form0);
  AddLogin(GetPasswordStore(0), form0);
  PasswordForm form1 = CreateTestPasswordForm(1);
  AddLogin(GetVerifierPasswordStore(), form1);
  AddLogin(GetPasswordStore(0), form1);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  RemoveLogins(GetPasswordStore(1));
  RemoveLogins(GetVerifierPasswordStore());
  ASSERT_TRUE(AwaitQuiescence());

  ASSERT_EQ(0, GetVerifierPasswordCount());
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());
}

// TCM ID - 3694311
// http://crbug.com/90460
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest, DISABLED_Merge) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());

  PasswordForm form0 = CreateTestPasswordForm(0);
  AddLogin(GetVerifierPasswordStore(), form0);
  AddLogin(GetPasswordStore(0), form0);
  PasswordForm form1 = CreateTestPasswordForm(1);
  AddLogin(GetVerifierPasswordStore(), form1);
  AddLogin(GetPasswordStore(0), form1);
  AddLogin(GetPasswordStore(1), form1);
  PasswordForm form2 = CreateTestPasswordForm(2);
  AddLogin(GetVerifierPasswordStore(), form2);
  AddLogin(GetPasswordStore(1), form2);
  ASSERT_TRUE(AwaitQuiescence());

  ASSERT_EQ(3, GetVerifierPasswordCount());
  ASSERT_TRUE(AllProfilesContainSamePasswordFormsAsVerifier());
}

// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest,
                       DISABLED_SetPassphraseAndThenSetupSync) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest,
                       SetPassphraseAndThenSetupSync) {
#endif
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  ASSERT_TRUE(GetClient(0)->SetupSync());
  SetPassphrase(0, kValidPassphrase, true);
  ASSERT_TRUE(GetClient(0)->AwaitPassphraseAccepted());
  ASSERT_TRUE(GetClient(0)->AwaitSyncCycleCompletion("Initial sync."));

  ASSERT_FALSE(GetClient(1)->SetupSync());
  SetPassphrase(1, kValidPassphrase, false);
  ASSERT_TRUE(GetClient(1)->AwaitPassphraseAccepted());
  ASSERT_TRUE(GetClient(1)->AwaitSyncCycleCompletion("Initial sync."));

  // Following ensures types are enabled and active (see bug 87572).
  browser_sync::ModelSafeRoutingInfo routes;
  GetClient(0)->service()->GetModelSafeRoutingInfo(&routes);
  ASSERT_EQ(browser_sync::GROUP_PASSWORD, routes[syncable::PASSWORDS]);
  routes.clear();
  GetClient(1)->service()->GetModelSafeRoutingInfo(&routes);
  ASSERT_EQ(browser_sync::GROUP_PASSWORD, routes[syncable::PASSWORDS]);
}

// TODO(sync): Enable after MockKeychain is fixed. http://crbug.com/89808.
#if defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest,
                       DISABLED_SetPassphraseTwice) {
#else
IN_PROC_BROWSER_TEST_F(TwoClientLivePasswordsSyncTest,
                       SetPassphraseTwice) {
#endif
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  SetPassphrase(0, kValidPassphrase, true);
  ASSERT_TRUE(GetClient(0)->AwaitPassphraseAccepted());
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  SetPassphrase(1, kValidPassphrase, false);
  ASSERT_TRUE(GetClient(1)->AwaitPassphraseAccepted());
  ASSERT_TRUE(GetClient(1)->AwaitSyncCycleCompletion("Set passphrase."));

  SetPassphrase(1, kValidPassphrase, false);
  ASSERT_TRUE(GetClient(1)->AwaitPassphraseAccepted());
  ASSERT_TRUE(GetClient(1)->AwaitSyncCycleCompletion("Set passphrase again."));
}
