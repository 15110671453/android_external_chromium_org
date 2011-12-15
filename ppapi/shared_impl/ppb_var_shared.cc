// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/shared_impl/ppb_var_shared.h"

#include <limits>

#include "ppapi/c/dev/ppb_var_array_buffer_dev.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/proxy_lock.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/shared_impl/var_tracker.h"

using ppapi::PpapiGlobals;
using ppapi::StringVar;

namespace ppapi {
namespace {


// PPB_Var methods -------------------------------------------------------------

void AddRefVar(PP_Var var) {
  ppapi::ProxyAutoLock lock;
  PpapiGlobals::Get()->GetVarTracker()->AddRefVar(var);
}

void ReleaseVar(PP_Var var) {
  ppapi::ProxyAutoLock lock;
  PpapiGlobals::Get()->GetVarTracker()->ReleaseVar(var);
}

PP_Var VarFromUtf8(const char* data, uint32_t len) {
  ppapi::ProxyAutoLock lock;
  return StringVar::StringToPPVar(data, len);
}

PP_Var VarFromUtf8_1_0(PP_Module /*module*/, const char* data, uint32_t len) {
  return VarFromUtf8(data, len);
}

const char* VarToUtf8(PP_Var var, uint32_t* len) {
  ppapi::ProxyAutoLock lock;
  StringVar* str = StringVar::FromPPVar(var);
  if (str) {
    *len = static_cast<uint32_t>(str->value().size());
    return str->value().c_str();
  }
  *len = 0;
  return NULL;
}

const PPB_Var var_interface = {
  &AddRefVar,
  &ReleaseVar,
  &VarFromUtf8,
  &VarToUtf8
};

const PPB_Var_1_0 var_interface1_0 = {
  &AddRefVar,
  &ReleaseVar,
  &VarFromUtf8_1_0,
  &VarToUtf8
};


// PPB_VarArrayBuffer_Dev methods ----------------------------------------------

PP_Var CreateArrayBufferVar(uint32_t size_in_bytes) {
  return PpapiGlobals::Get()->GetVarTracker()->MakeArrayBufferPPVar(
      size_in_bytes);
}

uint32_t ByteLength(struct PP_Var array) {
  ArrayBufferVar* buffer = ArrayBufferVar::FromPPVar(array);
  if (!buffer)
    return 0;
  return buffer->ByteLength();
}

void* Map(struct PP_Var array) {
  ArrayBufferVar* buffer = ArrayBufferVar::FromPPVar(array);
  if (!buffer)
    return NULL;
  return buffer->Map();
}

const PPB_VarArrayBuffer_Dev var_arraybuffer_interface = {
  &CreateArrayBufferVar,
  &ByteLength,
  &Map
};

}  // namespace

// static
const PPB_Var* PPB_Var_Shared::GetVarInterface() {
  return &var_interface;
}

// static
const PPB_Var_1_0* PPB_Var_Shared::GetVarInterface1_0() {
  return &var_interface1_0;
}

// static
const PPB_VarArrayBuffer_Dev* PPB_Var_Shared::GetVarArrayBufferInterface() {
 return &var_arraybuffer_interface;
}

}  // namespace ppapi

