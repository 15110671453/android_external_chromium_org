// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_TOOLS_TEST_SHELL_SIMPLE_DOM_STORAGE_SYSTEM_H_
#define WEBKIT_TOOLS_TEST_SHELL_SIMPLE_DOM_STORAGE_SYSTEM_H_
#pragma once

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "webkit/dom_storage/dom_storage_context.h"
#include "webkit/dom_storage/dom_storage_host.h"
#include "webkit/dom_storage/dom_storage_types.h"  // For the ENABLE flag.

namespace dom_storage {
class DomStorageContext;
class DomStorageHost;
}
namespace WebKit {
class WebStorageNamespace;
}

// Class that composes dom_storage classes together for use
// in simple single process environments like test_shell and DRT.
class SimpleDomStorageSystem {
 public:
  static SimpleDomStorageSystem& instance() { return *g_instance_; }

  SimpleDomStorageSystem();
  ~SimpleDomStorageSystem();

  // The Create<<>> calls are bound to WebKit api that the embedder
  // is responsible for implementing. These factories are called strictly
  // on the 'main' webkit thread. Ditto the methods on the returned
  // objects. SimplDomStorageSystem manufactures implementations of the
  // WebStorageNamespace and WebStorageArea interfaces that ultimately
  // plumb Get, Set, Remove, and Clear javascript calls to the dom_storage
  // classes. The caller (webkit/webcore) takes ownership of the returned
  // instances and will delete them when done.
  WebKit::WebStorageNamespace* CreateLocalStorageNamespace();
  WebKit::WebStorageNamespace* CreateSessionStorageNamespace();

 private:
  // Inner classes that implement the WebKit WebStorageNamespace and
  // WebStorageArea interfaces in terms of dom_storage classes.
  class NamespaceImpl;
  class AreaImpl;

  base::WeakPtrFactory<SimpleDomStorageSystem> weak_factory_;
  scoped_refptr<dom_storage::DomStorageContext> context_;
  scoped_ptr<dom_storage::DomStorageHost> host_;

  static SimpleDomStorageSystem* g_instance_;
};

#endif  // WEBKIT_TOOLS_TEST_SHELL_SIMPLE_DOM_STORAGE_SYSTEM_H_
