// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/value_store/value_store_frontend.h"

#include "chrome/browser/value_store/leveldb_value_store.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

class ValueStoreFrontend::Backend : public base::RefCountedThreadSafe<Backend> {
 public:
  explicit Backend(const FilePath& db_path) : storage_(NULL) {
    BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
        base::Bind(&ValueStoreFrontend::Backend::InitOnFileThread,
                   this, db_path));
  }

  void Get(const std::string& key,
           const ValueStoreFrontend::ReadCallback& callback) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
    ValueStore::ReadResult result = storage_->Get(key);

    // Extract the value from the ReadResult and pass ownership of it to the
    // callback.
    base::Value* value = NULL;
    if (!result->HasError())
      result->settings()->RemoveWithoutPathExpansion(key, &value);

    scoped_ptr<base::Value> passed_value(value);
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
        base::Bind(&ValueStoreFrontend::Backend::RunCallback,
                   this, callback, base::Passed(passed_value.Pass())));
  }

  void Set(const std::string& key, scoped_ptr<base::Value> value) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
    // We don't need the old value, so skip generating changes.
    storage_->Set(ValueStore::IGNORE_QUOTA | ValueStore::NO_GENERATE_CHANGES,
                  key, *value.get());
  }

  void Remove(const std::string& key) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
    storage_->Remove(key);
  }

 private:
  friend class base::RefCountedThreadSafe<Backend>;

  virtual ~Backend() {
    if (BrowserThread::CurrentlyOn(BrowserThread::FILE)) {
      delete storage_;
    } else {
      BrowserThread::DeleteSoon(BrowserThread::FILE, FROM_HERE, storage_);
    }
  }

  void InitOnFileThread(const FilePath& db_path) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
    DCHECK(!storage_);
    storage_ = LeveldbValueStore::Create(db_path);
  }

  void RunCallback(const ValueStoreFrontend::ReadCallback& callback,
                   scoped_ptr<base::Value> value) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    callback.Run(value.Pass());
  }

  // The actual ValueStore that handles persisting the data to disk. Used
  // exclusively on the FILE thread.
  LeveldbValueStore* storage_;

  DISALLOW_COPY_AND_ASSIGN(Backend);
};

ValueStoreFrontend::ValueStoreFrontend(const FilePath& db_path)
    : backend_(new Backend(db_path)) {
}

ValueStoreFrontend::~ValueStoreFrontend() {
  DCHECK(CalledOnValidThread());
}

void ValueStoreFrontend::Get(const std::string& key,
                             const ReadCallback& callback) {
  DCHECK(CalledOnValidThread());

  BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
      base::Bind(&ValueStoreFrontend::Backend::Get,
                 backend_, key, callback));
}

void ValueStoreFrontend::Set(const std::string& key,
                             scoped_ptr<base::Value> value) {
  DCHECK(CalledOnValidThread());

  BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
      base::Bind(&ValueStoreFrontend::Backend::Set,
                 backend_, key, base::Passed(value.Pass())));
}

void ValueStoreFrontend::Remove(const std::string& key) {
  DCHECK(CalledOnValidThread());

  BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
      base::Bind(&ValueStoreFrontend::Backend::Remove,
                 backend_, key));
}
