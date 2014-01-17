// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_PROCESS_MAP_FACTORY_H_
#define EXTENSIONS_BROWSER_PROCESS_MAP_FACTORY_H_

#include "base/compiler_specific.h"
#include "base/memory/singleton.h"
#include "components/browser_context_keyed_service/browser_context_keyed_service_factory.h"

namespace extensions {

class ProcessMap;

// Factory for ProcessMap objects. ProcessMap objects are shared between an
// incognito browser context and its master browser context.
class ProcessMapFactory : public BrowserContextKeyedServiceFactory {
 public:
  static ProcessMap* GetForBrowserContext(content::BrowserContext* context);

  static ProcessMapFactory* GetInstance();

 private:
  friend struct DefaultSingletonTraits<ProcessMapFactory>;

  ProcessMapFactory();
  virtual ~ProcessMapFactory();

  // BrowserContextKeyedServiceFactory implementation:
  virtual BrowserContextKeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const OVERRIDE;
  virtual content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProcessMapFactory);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_PROCESS_MAP_FACTORY_H_
