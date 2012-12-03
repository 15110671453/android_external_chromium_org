// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_MOCK_USER_CLOUD_POLICY_STORE_H_
#define CHROME_BROWSER_POLICY_MOCK_USER_CLOUD_POLICY_STORE_H_

#include "chrome/browser/policy/user_cloud_policy_store.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace policy {

class MockUserCloudPolicyStore : public UserCloudPolicyStore {
 public:
  MockUserCloudPolicyStore();
  virtual ~MockUserCloudPolicyStore();

  MOCK_METHOD1(Store, void(const enterprise_management::PolicyFetchResponse&));
  MOCK_METHOD0(Load, void(void));
  MOCK_METHOD0(LoadImmediately, void(void));
  MOCK_METHOD0(Clear, void(void));

  // Publish the protected members.
  using CloudPolicyStore::NotifyStoreLoaded;
  using CloudPolicyStore::NotifyStoreError;

  using CloudPolicyStore::policy_map_;
  using CloudPolicyStore::policy_;
  using CloudPolicyStore::status_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockUserCloudPolicyStore);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_MOCK_USER_CLOUD_POLICY_STORE_H_
