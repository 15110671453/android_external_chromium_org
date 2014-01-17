// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SERVICES_GCM_GCM_CLIENT_FACTORY_H_
#define CHROME_BROWSER_SERVICES_GCM_GCM_CLIENT_FACTORY_H_

#include "base/macros.h"

class Profile;

namespace gcm {

class GCMClient;

class GCMClientFactory {
 public:
  // Returns a single instance of GCMClient.
  // This should be called from IO thread.
  static GCMClient* GetClient();

  // Passes a mocked instance for testing purpose.
  typedef GCMClient* (*TestingFactoryFunction)();
  static void SetTestingFactory(TestingFactoryFunction factory);

 private:
  GCMClientFactory();
  ~GCMClientFactory();

  DISALLOW_COPY_AND_ASSIGN(GCMClientFactory);
};

}  // namespace gcm

#endif  // CHROME_BROWSER_SERVICES_GCM_GCM_CLIENT_FACTORY_H_
