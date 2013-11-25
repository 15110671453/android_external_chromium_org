// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/forwarding_policy_provider.h"

#include "components/policy/core/common/schema_map.h"
#include "components/policy/core/common/schema_registry.h"

namespace policy {

ForwardingPolicyProvider::ForwardingPolicyProvider(
    ConfigurationPolicyProvider* delegate)
    : delegate_(delegate), state_(WAITING_FOR_REGISTRY_READY) {
  delegate_->AddObserver(this);
  // Serve the initial |delegate_| policies.
  OnUpdatePolicy(delegate_);
}

ForwardingPolicyProvider::~ForwardingPolicyProvider() {
  delegate_->RemoveObserver(this);
}

void ForwardingPolicyProvider::Init(SchemaRegistry* registry) {
  ConfigurationPolicyProvider::Init(registry);
  if (registry->IsReady())
    OnSchemaRegistryReady();
}

bool ForwardingPolicyProvider::IsInitializationComplete(PolicyDomain domain)
    const {
  if (domain == POLICY_DOMAIN_CHROME)
    return delegate_->IsInitializationComplete(domain);
  // This provider keeps its own state for all the other domains.
  return state_ == READY;
}

void ForwardingPolicyProvider::RefreshPolicies() {
  delegate_->RefreshPolicies();
}

void ForwardingPolicyProvider::OnSchemaRegistryReady() {
  DCHECK_EQ(WAITING_FOR_REGISTRY_READY, state_);
  // This provider's registry is ready, meaning that it has all the initial
  // components schemas; the delegate's registry should also see them now,
  // since it's tracking the former.
  // Asking the delegate to RefreshPolicies now means that the next
  // OnUpdatePolicy from the delegate will have the initial policy for
  // components.
  if (!schema_map()->HasComponents()) {
    // If there are no component registered for this provider then there's no
    // need to reload.
    state_ = READY;
    OnUpdatePolicy(delegate_);
    return;
  }

  state_ = WAITING_FOR_REFRESH;
  RefreshPolicies();
}

void ForwardingPolicyProvider::OnSchemaRegistryUpdated(bool has_new_schemas) {
  if (state_ != READY)
    return;
  if (has_new_schemas) {
    RefreshPolicies();
  } else {
    // Remove the policies that were being served for the component that have
    // been removed. This is important so that update notifications are also
    // sent in case those component are reinstalled during the current session.
    OnUpdatePolicy(delegate_);
  }
}

void ForwardingPolicyProvider::OnUpdatePolicy(
    ConfigurationPolicyProvider* provider) {
  DCHECK_EQ(delegate_, provider);

  if (state_ == WAITING_FOR_REFRESH)
    state_ = READY;

  scoped_ptr<PolicyBundle> bundle(new PolicyBundle());
  if (state_ == READY) {
    bundle->CopyFrom(delegate_->policies());
    schema_map()->FilterBundle(bundle.get());
  } else {
    // Always forward the Chrome policy, even if the components are not ready
    // yet.
    const PolicyNamespace chrome_ns(POLICY_DOMAIN_CHROME, "");
    bundle->Get(chrome_ns).CopyFrom(delegate_->policies().Get(chrome_ns));
  }

  UpdatePolicy(bundle.Pass());
}

}  // namespace policy
