// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_ENTRY_H__
#define COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_ENTRY_H__

#include "base/strings/string16.h"
#include "base/time/time.h"

namespace autofill {

class AutofillKey {
 public:
  AutofillKey();
  AutofillKey(const base::string16& name, const base::string16& value);
  AutofillKey(const char* name, const char* value);
  AutofillKey(const AutofillKey& key);
  virtual ~AutofillKey();

  const base::string16& name() const { return name_; }
  const base::string16& value() const { return value_; }

  bool operator==(const AutofillKey& key) const;
  bool operator<(const AutofillKey& key) const;

 private:
  base::string16 name_;
  base::string16 value_;
};

class AutofillEntry {
 public:
  AutofillEntry(const AutofillKey& key,
                const base::Time& date_created,
                const base::Time& date_last_used);
  ~AutofillEntry();

  const AutofillKey& key() const { return key_; }
  const base::Time& date_created() const { return date_created_; }
  const base::Time& date_last_used() const { return date_last_used_; }

  bool operator==(const AutofillEntry& entry) const;
  bool operator<(const AutofillEntry& entry) const;

 private:
  AutofillKey key_;
  base::Time date_created_;
  base::Time date_last_used_;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_ENTRY_H__
