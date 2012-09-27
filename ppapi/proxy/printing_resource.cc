// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/printing_resource.h"

#include "ipc/ipc_message.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace ppapi {
namespace proxy {

PrintingResource::PrintingResource(Connection connection,
                                   PP_Instance instance)
    : PluginResource(connection, instance),
      print_settings_(NULL) {
}

PrintingResource::~PrintingResource() {
}

thunk::PPB_Printing_API* PrintingResource::AsPPB_Printing_API() {
  return this;
}

int32_t PrintingResource::GetDefaultPrintSettings(
    PP_PrintSettings_Dev* print_settings,
    scoped_refptr<TrackedCallback> callback) {
  if (!print_settings)
    return PP_ERROR_BADARGUMENT;

  if (TrackedCallback::IsPending(callback_))
    return PP_ERROR_INPROGRESS;

  if (!sent_create_to_browser())
    SendCreateToBrowser(PpapiHostMsg_Printing_Create());

  DCHECK(!print_settings_);
  print_settings_ = print_settings;
  callback_ = callback;

  CallBrowser(PpapiHostMsg_Printing_GetDefaultPrintSettings());
  return PP_OK_COMPLETIONPENDING;
}

void PrintingResource::OnReplyReceived(
    const ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  IPC_BEGIN_MESSAGE_MAP(PrintingResource, msg)
    PPAPI_DISPATCH_RESOURCE_REPLY(
        PpapiPluginMsg_Printing_GetDefaultPrintSettingsReply,
        OnPluginMsgGetDefaultPrintSettingsReply)
  IPC_END_MESSAGE_MAP()
}

void PrintingResource::OnPluginMsgGetDefaultPrintSettingsReply(
    const ResourceMessageReplyParams& params,
    const PP_PrintSettings_Dev& settings) {
  if (params.result() == PP_OK)
    *print_settings_ = settings;
  print_settings_ = NULL;

  // Notify the plugin of the new data.
  TrackedCallback::ClearAndRun(&callback_, params.result());
  // DANGER: May delete |this|!
}

}  // namespace proxy
}  // namespace ppapi
