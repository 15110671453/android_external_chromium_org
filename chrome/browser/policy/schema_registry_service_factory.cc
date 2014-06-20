// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/schema_registry_service_factory.h"

#include "base/logging.h"
#include "chrome/browser/policy/schema_registry_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/policy/core/common/schema.h"
#include "components/policy/core/common/schema_registry.h"
#include "content/public/browser/browser_context.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part_chromeos.h"
#include "chrome/browser/chromeos/login/users/user.h"
#include "chrome/browser/chromeos/login/users/user_manager.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/device_local_account_policy_service.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#endif

namespace policy {

#if defined(OS_CHROMEOS)
namespace {

DeviceLocalAccountPolicyBroker* GetBroker(content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);

  if (chromeos::ProfileHelper::IsSigninProfile(profile))
    return NULL;

  if (!chromeos::UserManager::IsInitialized()) {
    // Bail out in unit tests that don't have a UserManager.
    return NULL;
  }

  chromeos::UserManager* user_manager = chromeos::UserManager::Get();
  chromeos::User* user = user_manager->GetUserByProfile(profile);
  if (!user)
    return NULL;

  BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  DeviceLocalAccountPolicyService* service =
      connector->GetDeviceLocalAccountPolicyService();
  if (!service)
    return NULL;

  return service->GetBrokerForUser(user->email());
}

}  // namespace
#endif  // OS_CHROMEOS

// static
SchemaRegistryServiceFactory* SchemaRegistryServiceFactory::GetInstance() {
  return Singleton<SchemaRegistryServiceFactory>::get();
}

// static
SchemaRegistryService* SchemaRegistryServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return GetInstance()->GetForContextInternal(context);
}

// static
scoped_ptr<SchemaRegistryService>
SchemaRegistryServiceFactory::CreateForContext(
    content::BrowserContext* context,
    const Schema& chrome_schema,
    CombinedSchemaRegistry* global_registry) {
  return GetInstance()->CreateForContextInternal(
      context, chrome_schema, global_registry);
}

SchemaRegistryServiceFactory::SchemaRegistryServiceFactory()
    : BrowserContextKeyedBaseFactory(
          "SchemaRegistryService",
          BrowserContextDependencyManager::GetInstance()) {}

SchemaRegistryServiceFactory::~SchemaRegistryServiceFactory() {}

SchemaRegistryService* SchemaRegistryServiceFactory::GetForContextInternal(
    content::BrowserContext* context) {
  // Off-the-record Profiles get their policy from the main Profile's
  // PolicyService, and don't need their own SchemaRegistry nor any policy
  // providers.
  if (context->IsOffTheRecord())
    return NULL;
  RegistryMap::const_iterator it = registries_.find(context);
  CHECK(it != registries_.end());
  return it->second;
}

scoped_ptr<SchemaRegistryService>
SchemaRegistryServiceFactory::CreateForContextInternal(
    content::BrowserContext* context,
    const Schema& chrome_schema,
    CombinedSchemaRegistry* global_registry) {
  DCHECK(!context->IsOffTheRecord());
  DCHECK(registries_.find(context) == registries_.end());

  scoped_ptr<SchemaRegistry> registry;

#if defined(OS_CHROMEOS)
  DeviceLocalAccountPolicyBroker* broker = GetBroker(context);
  if (broker) {
    // The SchemaRegistry for a device-local account is owned by its
    // DeviceLocalAccountPolicyBroker, which uses the registry to fetch and
    // cache policy even if there is no active session for that account.
    // Use a ForwardingSchemaRegistry that wraps this SchemaRegistry.
    registry.reset(new ForwardingSchemaRegistry(broker->schema_registry()));
  }
#endif

  if (!registry)
    registry.reset(new SchemaRegistry);

  scoped_ptr<SchemaRegistryService> service(new SchemaRegistryService(
      registry.Pass(), chrome_schema, global_registry));
  registries_[context] = service.get();
  return service.Pass();
}

void SchemaRegistryServiceFactory::BrowserContextShutdown(
    content::BrowserContext* context) {
  if (context->IsOffTheRecord())
    return;
  RegistryMap::iterator it = registries_.find(context);
  if (it != registries_.end())
    it->second->Shutdown();
  else
    NOTREACHED();
}

void SchemaRegistryServiceFactory::BrowserContextDestroyed(
    content::BrowserContext* context) {
  registries_.erase(context);
  BrowserContextKeyedBaseFactory::BrowserContextDestroyed(context);
}

void SchemaRegistryServiceFactory::SetEmptyTestingFactory(
    content::BrowserContext* context) {}

void SchemaRegistryServiceFactory::CreateServiceNow(
    content::BrowserContext* context) {}

}  // namespace policy
