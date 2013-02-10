// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/support/test_webidbfactory.h"

#include "base/logging.h"
#include "webkit/support/webkit_support.h"

TestWebIDBFactory::TestWebIDBFactory() {
  // Create a new temp directory for Indexed DB storage, specific to this
  // factory. If this fails, WebKit uses in-memory storage.
  if (!indexed_db_dir_.CreateUniqueTempDir()) {
    LOG(WARNING) << "Failed to create a temp dir for Indexed DB, "
        "using in-memory storage.";
    DCHECK(indexed_db_dir_.path().empty());
  }
  data_dir_ = webkit_support::GetAbsoluteWebStringFromUTF8Path(
    indexed_db_dir_.path().AsUTF8Unsafe());
}

TestWebIDBFactory::~TestWebIDBFactory() {
}

void TestWebIDBFactory::getDatabaseNames(
    WebKit::WebIDBCallbacks* callbacks,
    const WebKit::WebSecurityOrigin& origin,
    WebKit::WebFrame* frame,
    const WebKit::WebString& data_dir) {
  GetFactory()->getDatabaseNames(callbacks, origin, frame,
                                 data_dir.isEmpty() ? data_dir_ : data_dir);
}

void TestWebIDBFactory::open(
    const WebKit::WebString& name,
    long long version,
    long long transaction_id,
    WebKit::WebIDBCallbacks* callbacks,
    WebKit::WebIDBDatabaseCallbacks* database_callbacks,
    const WebKit::WebSecurityOrigin& origin,
    WebKit::WebFrame* frame,
    const WebKit::WebString& data_dir) {
  GetFactory()->open(name, version, transaction_id, callbacks,
                     database_callbacks, origin, frame,
                     data_dir.isEmpty() ? data_dir_ : data_dir);
}

void TestWebIDBFactory::deleteDatabase(const WebKit::WebString& name,
                                       WebKit::WebIDBCallbacks* callbacks,
                                       const WebKit::WebSecurityOrigin& origin,
                                       WebKit::WebFrame* frame,
                                       const WebKit::WebString& dataDir) {
  GetFactory()->deleteDatabase(name, callbacks, origin, frame,
                               dataDir.isEmpty() ? data_dir_ : dataDir);
}

WebKit::WebIDBFactory* TestWebIDBFactory::GetFactory() {
  WebKit::WebIDBFactory* factory = factories_.Get();
  if (!factory) {
    factory = WebKit::WebIDBFactory::create();
    factories_.Set(factory);
  }
  return factory;
}
