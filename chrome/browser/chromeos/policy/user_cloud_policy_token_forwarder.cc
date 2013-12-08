// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/user_cloud_policy_token_forwarder.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/policy/user_cloud_policy_manager_chromeos.h"
#include "chrome/browser/signin/profile_oauth2_token_service.h"
#include "components/policy/core/common/cloud/cloud_policy_core.h"
#include "content/public/browser/notification_source.h"
#include "google_apis/gaia/gaia_constants.h"

namespace policy {

UserCloudPolicyTokenForwarder::UserCloudPolicyTokenForwarder(
    UserCloudPolicyManagerChromeOS* manager,
    ProfileOAuth2TokenService* token_service)
    : manager_(manager),
      token_service_(token_service) {
  // Start by waiting for the CloudPolicyService to be initialized, so that
  // we can check if it already has a DMToken or not.
  if (manager_->core()->service()->IsInitializationComplete()) {
    Initialize();
  } else {
    manager_->core()->service()->AddObserver(this);
  }
}

UserCloudPolicyTokenForwarder::~UserCloudPolicyTokenForwarder() {}

void UserCloudPolicyTokenForwarder::Shutdown() {
  request_.reset();
  token_service_->RemoveObserver(this);
  manager_->core()->service()->RemoveObserver(this);
}

void UserCloudPolicyTokenForwarder::OnRefreshTokenAvailable(
    const std::string& account_id) {
  RequestAccessToken();
}

void UserCloudPolicyTokenForwarder::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  manager_->OnAccessTokenAvailable(access_token);
  // All done here.
  Shutdown();
}

void UserCloudPolicyTokenForwarder::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  // This should seldom happen: if the user is signing in for the first time
  // then this was an online signin and network errors are unlikely; if the
  // user had already signed in before then he should have policy cached, and
  // RequestAccessToken() wouldn't have been invoked.
  // Still, something just went wrong (server 500, or something). Currently
  // we don't recover in this case, and we'll just try to register for policy
  // again on the next signin.
  // TODO(joaodasilva, atwilson): consider blocking signin when this happens,
  // so that the user has to try again before getting into the session. That
  // would guarantee that a session always has fresh policy, or at least
  // enforces a cached policy.
  Shutdown();
}

void UserCloudPolicyTokenForwarder::OnInitializationCompleted(
    CloudPolicyService* service) {
  Initialize();
}

void UserCloudPolicyTokenForwarder::Initialize() {
  if (manager_->IsClientRegistered()) {
    // We already have a DMToken, so no need to ask for an access token.
    // All done here.
    Shutdown();
    return;
  }

  if (token_service_->RefreshTokenIsAvailable(
          token_service_->GetPrimaryAccountId()))
    RequestAccessToken();
  else
    token_service_->AddObserver(this);
}

void UserCloudPolicyTokenForwarder::RequestAccessToken() {
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(GaiaConstants::kDeviceManagementServiceOAuth);
  request_ = token_service_->StartRequest(
      token_service_->GetPrimaryAccountId(), scopes, this);
}

}  // namespace policy
