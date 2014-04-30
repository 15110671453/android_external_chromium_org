// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/discardable_memory_emulated.h"

#include "base/lazy_instance.h"
#include "base/memory/discardable_memory_manager.h"

namespace base {
namespace {

base::LazyInstance<internal::DiscardableMemoryManager>::Leaky g_manager =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace internal {

DiscardableMemoryEmulated::DiscardableMemoryEmulated(size_t bytes)
    : bytes_(bytes),
      is_locked_(false) {
  g_manager.Pointer()->Register(this, bytes);
}

DiscardableMemoryEmulated::~DiscardableMemoryEmulated() {
  if (is_locked_)
    Unlock();
  g_manager.Pointer()->Unregister(this);
}

// static
void DiscardableMemoryEmulated::RegisterMemoryPressureListeners() {
  g_manager.Pointer()->RegisterMemoryPressureListener();
}

// static
void DiscardableMemoryEmulated::UnregisterMemoryPressureListeners() {
  g_manager.Pointer()->UnregisterMemoryPressureListener();
}

// static
void DiscardableMemoryEmulated::PurgeForTesting() {
  g_manager.Pointer()->PurgeAll();
}

bool DiscardableMemoryEmulated::Initialize() {
  return Lock() == DISCARDABLE_MEMORY_LOCK_STATUS_PURGED;
}

DiscardableMemoryLockStatus DiscardableMemoryEmulated::Lock() {
  DCHECK(!is_locked_);

  bool purged = false;
  if (!g_manager.Pointer()->AcquireLock(this, &purged))
    return DISCARDABLE_MEMORY_LOCK_STATUS_FAILED;

  is_locked_ = true;
  return purged ? DISCARDABLE_MEMORY_LOCK_STATUS_PURGED
                : DISCARDABLE_MEMORY_LOCK_STATUS_SUCCESS;
}

void DiscardableMemoryEmulated::Unlock() {
  DCHECK(is_locked_);
  g_manager.Pointer()->ReleaseLock(this);
  is_locked_ = false;
}

void* DiscardableMemoryEmulated::Memory() const {
  DCHECK(is_locked_);
  DCHECK(memory_);
  return memory_.get();
}

bool DiscardableMemoryEmulated::AllocateAndAcquireLock() {
  if (memory_)
    return true;

  memory_.reset(new uint8[bytes_]);
  return false;
}

void DiscardableMemoryEmulated::Purge() {
  memory_.reset();
}

}  // namespace internal
}  // namespace base
