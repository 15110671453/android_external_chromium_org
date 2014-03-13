// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_CRYPTOHOME_HOMEDIR_METHODS_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_CRYPTOHOME_HOMEDIR_METHODS_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "chrome/browser/chromeos/login/cryptohome/cryptohome_parameters.h"
#include "chromeos/attestation/attestation_constants.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace cryptohome {
// This class manages calls to Cryptohome service's home directory methods:
// Mount, CheckKey, Add/UpdateKey.
class HomedirMethods {
 public:
  // A callback type which is called back on the UI thread when the results of
  // method calls are ready.
  typedef base::Callback<void(bool success, MountError return_code)> Callback;
  typedef base::Callback<
      void(bool success, MountError return_code, const std::string& mount_hash)>
      MountCallback;

  virtual ~HomedirMethods() {}

  // Asks cryptohomed to attempt authorization for user identified by |id| using
  // |auth|. This can be used to unlock a user session.
  virtual void CheckKeyEx(const Identification& id,
                          const Authorization& auth,
                          const Callback& callback) = 0;

  // Asks cryptohomed to find the cryptohome for user identified by |id| and
  // then mount it using |auth| to unlock the key.
  // If the |create_keys| are not given and no cryptohome exists for |id|,
  // the expected result is
  // callback.Run(false, kCryptohomeMountErrorUserDoesNotExist, string()).
  // Otherwise, the normal range of return codes is expected.
  virtual void MountEx(const Identification& id,
                       const Authorization& auth,
                       const MountParameters& request,
                       const MountCallback& callback) = 0;

  // Asks cryptohomed to try to add another |key| for user identified by |id|
  // using |auth| to unlock the key.
  // |clobber_if_exist| governs action if key with same label already exists for
  // this user. if |true| old key will be replaced, if  |false| old key will be
  // preserved.
  // Key used in |auth| should have PRIV_ADD privilege.
  // |callback| will be called with status info on completion.
  virtual void AddKeyEx(const Identification& id,
                        const Authorization& auth,
                        const KeyDefinition& key,
                        bool clobber_if_exist,
                        const Callback& callback) = 0;

  // Asks cryptohomed to update |key| for user identified by |id| using |auth|
  // to unlock the key.
  // Label for |auth| and |key| have to be the same.
  // Key used in |auth| should have PRIV_AUTHORIZED_UPDATE privilege.
  // |signature| is used by cryptohome to verify the authentity of new key.
  // |callback| will be called with status info on completion.
  virtual void UpdateKeyEx(const Identification& id,
                           const Authorization& auth,
                           const KeyDefinition& key,
                           const std::string& signature,
                           const Callback& callback) = 0;

  // Creates the global HomedirMethods instance.
  static void Initialize();

  // Similar to Initialize(), but can inject an alternative
  // HomedirMethods such as MockHomedirMethods for testing.
  // The injected object will be owned by the internal pointer and deleted
  // by Shutdown().
  static void InitializeForTesting(HomedirMethods* homedir_methods);

  // Destroys the global HomedirMethods instance if it exists.
  static void Shutdown();

  // Returns a pointer to the global HomedirMethods instance.
  // Initialize() should already have been called.
  static HomedirMethods* GetInstance();
};

}  // namespace cryptohome

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_CRYPTOHOME_HOMEDIR_METHODS_H_
