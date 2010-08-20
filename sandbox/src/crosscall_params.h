// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_SRC_CROSSCALL_PARAMS_H__
#define SANDBOX_SRC_CROSSCALL_PARAMS_H__

#include <windows.h>
#include <lmaccess.h>

#include <memory>

#include "base/basictypes.h"
#include "sandbox/src/internal_types.h"
#include "sandbox/src/sandbox_types.h"

namespace {

// Increases |value| until there is no need for padding given the 2*pointer
// alignment on the platform. Returns the increased value.
// NOTE: This might not be good enough for some buffer. The OS might want the
// structure inside the buffer to be aligned also.
size_t Align(size_t value) {
  size_t alignment = sizeof(ULONG_PTR) * 2;
  return ((value + alignment - 1) / alignment) * alignment;
}

}
// This header is part of CrossCall: the sandbox inter-process communication.
// This header defines the basic types used both in the client IPC and in the
// server IPC code. CrossCallParams and ActualCallParams model the input
// parameters of an IPC call and CrossCallReturn models the output params and
// the return value.
//
// An IPC call is defined by its 'tag' which is a (uint32) unique identifier
// that is used to route the IPC call to the proper server. Every tag implies
// a complete call signature including the order and type of each parameter.
//
// Like most IPC systems. CrossCall is designed to take as inputs 'simple'
// types such as integers and strings. Classes, generic arrays or pointers to
// them are not supported.
//
// Another limitation of CrossCall is that the return value and output
// parameters can only be uint32 integers. Returning complex structures or
// strings is not supported.

namespace sandbox {

// max number of extended return parameters. See CrossCallReturn
const size_t kExtendedReturnCount = 8;

// Union of multiple types to be used as extended results
// in the CrossCallReturn.
union MultiType {
  uint32 unsigned_int;
  void* pointer;
  HANDLE handle;
  ULONG_PTR ulong_ptr;
};

// Maximum number of IPC parameters currently supported.
// To increase this value, we have to:
//  - Add another Callback typedef to Dispatcher.
//  - Add another case to the switch on SharedMemIPCServer::InvokeCallback.
//  - Add another case to the switch in GetActualAndMaxBufferSize
const int kMaxIpcParams = 9;

// Contains the information about a parameter in the ipc buffer.
struct ParamInfo {
  ArgType type_;
  ptrdiff_t offset_;
  size_t size_;
};

// Models the return value and the return parameters of an IPC call
// currently limited to one status code and eight generic return values
// which cannot be pointers to other data. For x64 ports this structure
// might have to use other integer types.
struct CrossCallReturn {
  // the IPC tag. It should match the original IPC tag.
  uint32 tag;
  // The result of the IPC operation itself.
  ResultCode call_outcome;
  // the result of the IPC call as executed in the server. The interpretation
  // of this value depends on the specific service.
  union {
    NTSTATUS nt_status;
    DWORD    win32_result;
  };
  // for calls that should return a windows handle. It is found here.
  HANDLE handle;
  // Number of extended return values.
  uint32 extended_count;
  // The array of extended values.
  MultiType extended[kExtendedReturnCount];
};

// CrossCallParams base class that models the input params all packed in a
// single compact memory blob. The representation can vary but in general a
// given child of this class is meant to represent all input parameters
// necessary to make a IPC call.
//
// This class cannot have virtual members because its assumed the IPC
// parameters start from the 'this' pointer to the end, which is defined by
// one of the subclasses
//
// Objects of this class cannot be constructed directly. Only derived
// classes have the proper knowledge to construct it.
class CrossCallParams {
 public:
  // Returns the tag (ipc unique id) associated with this IPC.
  uint32 GetTag() const {
    return tag_;
  }

  // Returns the beggining of the buffer where the IPC params can be stored.
  // prior to an IPC call
  const void* GetBuffer() const {
    return this;
  }

  // Returns how many parameter this IPC call should have.
  const size_t GetParamsCount() const {
    return params_count_;
  }

  // Returns a pointer to the CrossCallReturn structure.
  CrossCallReturn* GetCallReturn() {
    return &call_return;
  }

  // Returns TRUE if this call contains InOut parameters.
  const bool IsInOut() const {
    return (1 == is_in_out_);
  }

  // Tells the CrossCall object if it contains InOut parameters.
  void SetIsInOut(bool value) {
    if (value)
      is_in_out_ = 1;
    else
      is_in_out_ = 0;
  }

 protected:
  // constructs the IPC call params. Called only from the derived classes
  explicit CrossCallParams(uint32 tag, size_t params_count)
      : tag_(tag),
        params_count_(params_count),
        is_in_out_(0) {
  }

 private:
  uint32 tag_;
  uint32 is_in_out_;
  CrossCallReturn call_return;
  const size_t params_count_;
  DISALLOW_COPY_AND_ASSIGN(CrossCallParams);
};

// ActualCallParams models an specific IPC call parameters with respect to the
// storage allocation that the packed parameters should need.
// NUMBER_PARAMS: the number of parameters, valid from 1 to N
// BLOCK_SIZE: the total storage that the NUMBER_PARAMS parameters can take,
// typically the block size is defined by the channel size of the underlying
// ipc mechanism.
// In practice this class is used to levergage C++ capacity to properly
// calculate sizes and displacements given the possibility of the packed params
// blob to be complex.
//
// As is, this class assumes that the layout of the blob is as follows. Assume
// that NUMBER_PARAMS = 2 and a 32-bit build:
//
// [ tag                4 bytes]
// [ IsOnOut            4 bytes]
// [ call return       52 bytes]
// [ params count       4 bytes]
// [ parameter 0 type   4 bytes]
// [ parameter 0 offset 4 bytes] ---delta to ---\
// [ parameter 0 size   4 bytes]                |
// [ parameter 1 type   4 bytes]                |
// [ parameter 1 offset 4 bytes] ---------------|--\
// [ parameter 1 size   4 bytes]                |  |
// [ parameter 2 type   4 bytes]                |  |
// [ parameter 2 offset 4 bytes] ----------------------\
// [ parameter 2 size   4 bytes]                |  |   |
// |---------------------------|                |  |   |
// | value 0     (x bytes)     | <--------------/  |   |
// | value 1     (y bytes)     | <-----------------/   |
// |                           |                       |
// | end of buffer             | <---------------------/
// |---------------------------|
//
// Note that the actual number of params is NUMBER_PARAMS + 1
// so that the size of each actual param can be computed from the difference
// between one parameter and the next down. The offset of the last param
// points to the end of the buffer and the type and size are undefined.
//
template <size_t NUMBER_PARAMS, size_t BLOCK_SIZE>
class ActualCallParams : public CrossCallParams {
 public:
  // constructor. Pass the ipc unique tag as input
  explicit ActualCallParams(uint32 tag)
      : CrossCallParams(tag, NUMBER_PARAMS) {
    param_info_[0].offset_ = parameters_ - reinterpret_cast<char*>(this);
  }

  // Testing-only constructor. Allows setting the |number_params| to a
  // wrong value.
  ActualCallParams(uint32 tag, uint32 number_params)
      : CrossCallParams(tag, number_params) {
    param_info_[0].offset_ = parameters_ - reinterpret_cast<char*>(this);
  }

  // Testing-only method. Allows setting the apparent size to a wrong value.
  // returns the previous size.
  size_t OverrideSize(size_t new_size) {
    size_t previous_size = param_info_[NUMBER_PARAMS].offset_;
    param_info_[NUMBER_PARAMS].offset_ = new_size;
    return previous_size;
  }

  // Copies each paramter into the internal buffer. For each you must supply:
  // index: 0 for the first param, 1 for the next an so on
  bool CopyParamIn(size_t index, const void* parameter_address, size_t size,
                   bool is_in_out, ArgType type) {
    if (index >= NUMBER_PARAMS) {
      return false;
    }

    if (kuint32max == size) {
      // Memory error while getting the size.
      return false;
    }

    if (size && !parameter_address) {
      return false;
    }

    if (param_info_[index].offset_ > sizeof(*this)) {
      // it does not fit, abort copy
      return false;
    }

    char* dest = reinterpret_cast<char*>(this) +  param_info_[index].offset_;

    // We might be touching user memory, this has to be done from inside a try
    // except.
    __try {
      memcpy(dest, parameter_address, size);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
      return false;
    }

    // Set the flag to tell the broker to update the buffer once the call is
    // made.
    if (is_in_out)
      SetIsInOut(true);

    param_info_[index + 1].offset_ = Align(param_info_[index].offset_ +
                                                size);
    param_info_[index].size_ = size;
    param_info_[index].type_ = type;
    return true;
  }

  // Returns a pointer to a parameter in the memory section.
  void* GetParamPtr(size_t index) {
    return reinterpret_cast<char*>(this) + param_info_[index].offset_;
  }

  // returns the total size of the buffer. Only valid once all the paramters
  // have been copied in with CopyParamIn
  size_t GetSize() const {
    return param_info_[NUMBER_PARAMS].offset_;
  }

 protected:
  ActualCallParams() : CrossCallParams(0, NUMBER_PARAMS) { }

 private:
  ParamInfo param_info_[NUMBER_PARAMS + 1];
  char parameters_[BLOCK_SIZE - sizeof(CrossCallParams)
                   - sizeof(ParamInfo) * (NUMBER_PARAMS + 1)];
  DISALLOW_COPY_AND_ASSIGN(ActualCallParams);
};

COMPILE_ASSERT(sizeof(ActualCallParams<1, 1024>) == 1024, bad_size_buffer);
COMPILE_ASSERT(sizeof(ActualCallParams<2, 1024>) == 1024, bad_size_buffer);
COMPILE_ASSERT(sizeof(ActualCallParams<3, 1024>) == 1024, bad_size_buffer);

}  // namespace sandbox

#endif  // SANDBOX_SRC_CROSSCALL_PARAMS_H__
