// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/prefs/pref_change_registrar.h"

#include "base/bind.h"
// TODO(battre): Delete this. See crbug.com/373435.
#include "base/debug/alias.h"
#include "base/logging.h"
#include "base/prefs/pref_service.h"

PrefChangeRegistrar::PrefChangeRegistrar() : service_(NULL) {}

PrefChangeRegistrar::~PrefChangeRegistrar() {
  // If you see an invalid memory access in this destructor, this
  // PrefChangeRegistrar might be subscribed to an OffTheRecordProfileImpl that
  // has been destroyed. This should not happen any more but be warned.
  // Feel free to contact battre@chromium.org in case this happens.
  RemoveAll();
}

void PrefChangeRegistrar::Init(PrefService* service) {
  DCHECK(IsEmpty() || service_ == service);
  service_ = service;
}

void PrefChangeRegistrar::Add(const char* path,
                              const base::Closure& obs) {
  Add(path, base::Bind(&PrefChangeRegistrar::InvokeUnnamedCallback, obs));
}

void PrefChangeRegistrar::Add(const char* path,
                              const NamedChangeCallback& obs) {
  if (!service_) {
    NOTREACHED();
    return;
  }
  DCHECK(!IsObserved(path)) << "Already had this pref registered.";

  service_->AddPrefObserver(path, this);
  observers_[path] = obs;
}

void PrefChangeRegistrar::Remove(const char* path) {
  DCHECK(IsObserved(path));

  observers_.erase(path);
  service_->RemovePrefObserver(path, this);
}

void PrefChangeRegistrar::RemoveAll() {
  // TODO(battre): Delete this. See crbug.com/373435.
  if (!observers_.empty() && !pref_service_destruction_.empty()) {
    // The PrefService is already destroyed, so the following call to
    // service_->RemovePrefObserver would crash anyway. When the PrefService
    // was destroyed, it stored a stack trace of the destruction in
    // pref_service_destruction_. We save this on the stack in the minidump to
    // understand what happens.
    char tmp[2048] = {};
    strncat(tmp, pref_service_destruction_.c_str(), sizeof(tmp) - 1u);
    base::debug::Alias(tmp);
    CHECK(false) << tmp;
  }

  for (ObserverMap::const_iterator it = observers_.begin();
       it != observers_.end(); ++it) {
    service_->RemovePrefObserver(it->first.c_str(), this);
  }

  observers_.clear();
}

bool PrefChangeRegistrar::IsEmpty() const {
  return observers_.empty();
}

bool PrefChangeRegistrar::IsObserved(const std::string& pref) {
  return observers_.find(pref) != observers_.end();
}

bool PrefChangeRegistrar::IsManaged() {
  for (ObserverMap::const_iterator it = observers_.begin();
       it != observers_.end(); ++it) {
    const PrefService::Preference* pref =
        service_->FindPreference(it->first.c_str());
    if (pref && pref->IsManaged())
      return true;
  }
  return false;
}

void PrefChangeRegistrar::OnPreferenceChanged(PrefService* service,
                                              const std::string& pref) {
  if (IsObserved(pref))
    observers_[pref].Run(pref);
}

// TODO(battre): Delete this. See crbug.com/373435.
void PrefChangeRegistrar::SetPrefServiceDestructionTrace(
    const std::string& stack_trace) {
  pref_service_destruction_ = stack_trace;
}

void PrefChangeRegistrar::InvokeUnnamedCallback(const base::Closure& callback,
                                                const std::string& pref_name) {
  callback.Run();
}

PrefService* PrefChangeRegistrar::prefs() {
  return service_;
}

const PrefService* PrefChangeRegistrar::prefs() const {
  return service_;
}
