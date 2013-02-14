// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/device_orientation/data_fetcher_impl_android.h"

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "content/browser/device_orientation/orientation.h"
#include "jni/DeviceOrientation_jni.h"

using base::android::AttachCurrentThread;

namespace content {

namespace {

// This should match ProviderImpl::kDesiredSamplingIntervalMs.
// TODO(husky): Make that constant public so we can use it directly.
const int kPeriodInMilliseconds = 100;

}  // namespace

DataFetcherImplAndroid::DataFetcherImplAndroid() {
  device_orientation_.Reset(
      Java_DeviceOrientation_getInstance(AttachCurrentThread()));
}

void DataFetcherImplAndroid::Init(JNIEnv* env) {
  bool result = RegisterNativesImpl(env);
  DCHECK(result);
}

DataFetcher* DataFetcherImplAndroid::Create() {
  scoped_ptr<DataFetcherImplAndroid> fetcher(new DataFetcherImplAndroid);
  if (fetcher->Start(kPeriodInMilliseconds))
    return fetcher.release();

  LOG(ERROR) << "DataFetcherImplAndroid::Start failed!";
  return NULL;
}

DataFetcherImplAndroid::~DataFetcherImplAndroid() {
  Stop();
}

const DeviceData* DataFetcherImplAndroid::GetDeviceData(
    DeviceData::Type type) {
  if (type != DeviceData::kTypeOrientation)
    return NULL;
  return GetOrientation();
}

const Orientation* DataFetcherImplAndroid::GetOrientation() {
  // Do we have a new orientation value? (It's safe to do this outside the lock
  // because we only skip the lock if the value is null. We always enter the
  // lock if we're going to make use of the new value.)
  if (next_orientation_.get()) {
    base::AutoLock autolock(next_orientation_lock_);
    next_orientation_.swap(current_orientation_);
  }
  if (!current_orientation_.get())
    return new Orientation();
  return current_orientation_.get();
}

void DataFetcherImplAndroid::GotOrientation(
    JNIEnv*, jobject, double alpha, double beta, double gamma) {
  base::AutoLock autolock(next_orientation_lock_);

  Orientation* orientation = new Orientation();
  orientation->set_alpha(alpha);
  orientation->set_beta(beta);
  orientation->set_gamma(gamma);
  orientation->set_absolute(true);
  next_orientation_ = orientation;
}

bool DataFetcherImplAndroid::Start(int rate_in_milliseconds) {
  DCHECK(!device_orientation_.is_null());
  return Java_DeviceOrientation_start(AttachCurrentThread(),
                                      device_orientation_.obj(),
                                      reinterpret_cast<jint>(this),
                                      rate_in_milliseconds);
}

void DataFetcherImplAndroid::Stop() {
  DCHECK(!device_orientation_.is_null());
  Java_DeviceOrientation_stop(AttachCurrentThread(), device_orientation_.obj());
}

}  // namespace content
