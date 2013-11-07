// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/schema_map.h"

#include "base/values.h"
#include "chrome/browser/policy/policy_bundle.h"
#include "chrome/browser/policy/policy_map.h"

namespace policy {

SchemaMap::SchemaMap() {}

SchemaMap::SchemaMap(DomainMap& map) {
  map_.swap(map);
}

SchemaMap::~SchemaMap() {}

const DomainMap& SchemaMap::GetDomains() const {
  return map_;
}

const ComponentMap* SchemaMap::GetComponents(PolicyDomain domain) const {
  DomainMap::const_iterator it = map_.find(domain);
  return it == map_.end() ? NULL : &it->second;
}

const Schema* SchemaMap::GetSchema(const PolicyNamespace& ns) const {
  const ComponentMap* map = GetComponents(ns.domain);
  if (!map)
    return NULL;
  ComponentMap::const_iterator it = map->find(ns.component_id);
  return it == map->end() ? NULL : &it->second;
}

void SchemaMap::FilterBundle(PolicyBundle* bundle) const {
  for (PolicyBundle::iterator it = bundle->begin(); it != bundle->end(); ++it) {
    // Chrome policies are not filtered, so that typos appear in about:policy.
    // Everything else gets filtered, so that components only see valid policy.
    if (it->first.domain == POLICY_DOMAIN_CHROME)
      continue;

    const Schema* schema = GetSchema(it->first);

    if (!schema) {
      it->second->Clear();
      continue;
    }

    // TODO(joaodasilva): if a component is registered but doesn't have a schema
    // then its policies aren't filtered. This behavior is enabled to allow a
    // graceful update of the Legacy Browser Support extension; it'll be removed
    // in a future release. http://crbug.com/240704
    if (!schema->valid())
      continue;

    PolicyMap* map = it->second;
    for (PolicyMap::const_iterator it_map = map->begin();
         it_map != map->end();) {
      const std::string& policy_name = it_map->first;
      const base::Value* policy_value = it_map->second.value;
      Schema policy_schema = schema->GetProperty(policy_name);
      ++it_map;
      if (!policy_value || !policy_schema.Validate(*policy_value))
        map->Erase(policy_name);
    }
  }
}

}  // namespace policy
