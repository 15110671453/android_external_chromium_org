// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From ppb_udp_socket.idl modified Tue Aug 20 08:13:36 2013.

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_udp_socket.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_udp_socket_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Instance instance) {
  VLOG(4) << "PPB_UDPSocket::Create()";
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateUDPSocket(instance);
}

PP_Bool IsUDPSocket(PP_Resource resource) {
  VLOG(4) << "PPB_UDPSocket::IsUDPSocket()";
  EnterResource<PPB_UDPSocket_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

int32_t Bind(PP_Resource udp_socket,
             PP_Resource addr,
             struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::Bind()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Bind(addr, enter.callback()));
}

PP_Resource GetBoundAddress(PP_Resource udp_socket) {
  VLOG(4) << "PPB_UDPSocket::GetBoundAddress()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, true);
  if (enter.failed())
    return 0;
  return enter.object()->GetBoundAddress();
}

int32_t RecvFrom(PP_Resource udp_socket,
                 char* buffer,
                 int32_t num_bytes,
                 PP_Resource* addr,
                 struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::RecvFrom()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->RecvFrom(buffer,
                                                  num_bytes,
                                                  addr,
                                                  enter.callback()));
}

int32_t SendTo(PP_Resource udp_socket,
               const char* buffer,
               int32_t num_bytes,
               PP_Resource addr,
               struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::SendTo()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->SendTo(buffer,
                                                num_bytes,
                                                addr,
                                                enter.callback()));
}

void Close(PP_Resource udp_socket) {
  VLOG(4) << "PPB_UDPSocket::Close()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, true);
  if (enter.failed())
    return;
  enter.object()->Close();
}

int32_t SetOption(PP_Resource udp_socket,
                  PP_UDPSocket_Option name,
                  struct PP_Var value,
                  struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::SetOption()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->SetOption(name,
                                                   value,
                                                   enter.callback()));
}

const PPB_UDPSocket_1_0 g_ppb_udpsocket_thunk_1_0 = {
  &Create,
  &IsUDPSocket,
  &Bind,
  &GetBoundAddress,
  &RecvFrom,
  &SendTo,
  &Close,
  &SetOption
};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_UDPSocket_1_0* GetPPB_UDPSocket_1_0_Thunk() {
  return &g_ppb_udpsocket_thunk_1_0;
}

}  // namespace thunk
}  // namespace ppapi
