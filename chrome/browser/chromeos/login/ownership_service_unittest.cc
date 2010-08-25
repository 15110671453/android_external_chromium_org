// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ownership_service.h"

#include <string>

#include "base/crypto/rsa_private_key.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/nss_util.h"
#include "base/scoped_ptr.h"
#include "base/scoped_temp_dir.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/chromeos/login/mock_owner_key_utils.h"
#include "chrome/browser/chromeos/login/owner_manager_unittest.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::base::RSAPrivateKey;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgumentPointee;
using ::testing::_;


namespace chromeos {

class OwnershipServiceTest : public ::testing::Test {
 public:
  OwnershipServiceTest()
      : message_loop_(MessageLoop::TYPE_UI),
        ui_thread_(ChromeThread::UI, &message_loop_),
        file_thread_(ChromeThread::FILE),
        mock_(new MockKeyUtils),
        injector_(mock_) /* injector_ takes ownership of mock_ */ {
  }
  virtual ~OwnershipServiceTest() {}

  virtual void SetUp() {
    base::OpenPersistentNSSDB();  // TODO(cmasone): use test DB instead
    fake_private_key_.reset(RSAPrivateKey::Create(256));
    ASSERT_TRUE(fake_private_key_->ExportPublicKey(&fake_public_key_));

    // Mimic ownership.
    ASSERT_TRUE(tmpdir_.CreateUniqueTempDir());
    ASSERT_TRUE(file_util::CreateTemporaryFileInDir(tmpdir_.path(), &tmpfile_));

    file_thread_.Start();
    OwnerKeyUtils::set_factory(&injector_);
    service_.reset(new OwnershipService);  // must happen AFTER set_factory().

  }

  virtual void TearDown() {
    OwnerKeyUtils::set_factory(NULL);
    service_.reset(NULL);
  }

  void StartUnowned() {
    file_util::Delete(tmpfile_, false);
  }

  ScopedTempDir tmpdir_;
  FilePath tmpfile_;

  MessageLoop message_loop_;
  ChromeThread ui_thread_;
  ChromeThread file_thread_;

  std::vector<uint8> fake_public_key_;
  scoped_ptr<RSAPrivateKey> fake_private_key_;

  MockKeyUtils* mock_;
  MockInjector injector_;
  scoped_ptr<OwnershipService> service_;
};

TEST_F(OwnershipServiceTest, IsOwned) {
  EXPECT_CALL(*mock_, GetOwnerKeyFilePath())
      .WillRepeatedly(Return(tmpfile_));
  EXPECT_TRUE(service_->IsAlreadyOwned());
}

TEST_F(OwnershipServiceTest, IsUnowned) {
  StartUnowned();

  EXPECT_CALL(*mock_, GetOwnerKeyFilePath())
      .WillRepeatedly(Return(tmpfile_));
  EXPECT_FALSE(service_->IsAlreadyOwned());
}

TEST_F(OwnershipServiceTest, LoadOwnerKeyUnowned) {
  StartUnowned();

  EXPECT_CALL(*mock_, GetOwnerKeyFilePath())
      .WillRepeatedly(Return(tmpfile_));
  EXPECT_FALSE(service_->StartLoadOwnerKeyAttempt());
}

TEST_F(OwnershipServiceTest, LoadOwnerKeyFail) {
  MockKeyLoadObserver loader;
  EXPECT_CALL(*mock_, GetOwnerKeyFilePath())
      .WillRepeatedly(Return(tmpfile_));
  EXPECT_CALL(*mock_, ImportPublicKey(tmpfile_, _))
      .WillOnce(Return(false))
      .RetiresOnSaturation();

  EXPECT_TRUE(service_->StartLoadOwnerKeyAttempt());

  // Run remaining events, until ExportPublicKeyViaDbus().
  message_loop_.Run();
}

TEST_F(OwnershipServiceTest, LoadOwnerKey) {
  MockKeyLoadObserver loader;
  loader.ExpectKeyFetchSuccess(true);

  EXPECT_CALL(*mock_, GetOwnerKeyFilePath())
      .WillRepeatedly(Return(tmpfile_));
  EXPECT_CALL(*mock_, ImportPublicKey(tmpfile_, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(fake_public_key_),
                      Return(true)))
      .RetiresOnSaturation();
  EXPECT_TRUE(service_->StartLoadOwnerKeyAttempt());

  message_loop_.Run();
}

TEST_F(OwnershipServiceTest, TakeOwnershipAlreadyOwned) {
  EXPECT_CALL(*mock_, GetOwnerKeyFilePath())
      .WillRepeatedly(Return(tmpfile_));
  EXPECT_FALSE(service_->StartTakeOwnershipAttempt());
}

TEST_F(OwnershipServiceTest, AttemptKeyGeneration) {
  // We really only care that we initiate key generation here;
  // actual key-generation paths are tested in owner_manager_unittest.cc
  StartUnowned();
  MockKeyLoadObserver loader;
  loader.ExpectKeyFetchSuccess(false);

  EXPECT_CALL(*mock_, GenerateKeyPair())
      .WillOnce(Return(reinterpret_cast<RSAPrivateKey*>(NULL)))
      .RetiresOnSaturation();
  EXPECT_CALL(*mock_, GetOwnerKeyFilePath())
      .WillRepeatedly(Return(tmpfile_));

  EXPECT_TRUE(service_->StartTakeOwnershipAttempt());

  message_loop_.Run();
}

TEST_F(OwnershipServiceTest, NotYetOwnedVerify) {
  StartUnowned();

  EXPECT_CALL(*mock_, GetOwnerKeyFilePath())
      .WillRepeatedly(Return(tmpfile_));
  MockKeyUser delegate(OwnerManager::KEY_UNAVAILABLE);
  EXPECT_FALSE(service_->StartVerifyAttempt("", "", &delegate));
}

TEST_F(OwnershipServiceTest, GetKeyFailDuringVerify) {
  MockKeyLoadObserver loader;
  loader.ExpectKeyFetchSuccess(false);

  EXPECT_CALL(*mock_, GetOwnerKeyFilePath())
      .WillRepeatedly(Return(tmpfile_));
  EXPECT_CALL(*mock_, ImportPublicKey(tmpfile_, _))
      .WillOnce(Return(false))
      .RetiresOnSaturation();

  MockKeyUser delegate(OwnerManager::KEY_UNAVAILABLE);
  EXPECT_TRUE(service_->StartVerifyAttempt("", "", &delegate));

  message_loop_.Run();
}

TEST_F(OwnershipServiceTest, GetKeyAndVerify) {
  MockKeyLoadObserver loader;
  loader.ExpectKeyFetchSuccess(true);
  loader.SetQuitOnKeyFetch(false);

  EXPECT_CALL(*mock_, GetOwnerKeyFilePath())
      .WillRepeatedly(Return(tmpfile_));
  EXPECT_CALL(*mock_, ImportPublicKey(tmpfile_, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(fake_public_key_),
                      Return(true)))
      .RetiresOnSaturation();

  MockKeyUser delegate(OwnerManager::SUCCESS);
  EXPECT_TRUE(service_->StartVerifyAttempt("", "", &delegate));

  message_loop_.Run();
}

}  // namespace chromeos
