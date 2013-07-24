// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_url_loader_host.h"

#include "content/renderer/pepper/renderer_ppapi_host_impl.h"
#include "content/renderer/pepper/url_response_info_util.h"
#include "net/base/net_errors.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLLoader.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/platform/WebURLResponse.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"
#include "third_party/WebKit/public/web/WebSecurityOrigin.h"
#include "third_party/WebKit/public/web/WebURLLoaderOptions.h"
#include "webkit/plugins/ppapi/ppapi_plugin_instance_impl.h"
#include "webkit/plugins/ppapi/url_request_info_util.h"

using WebKit::WebFrame;
using WebKit::WebString;
using WebKit::WebURL;
using WebKit::WebURLError;
using WebKit::WebURLLoader;
using WebKit::WebURLLoaderOptions;
using WebKit::WebURLRequest;
using WebKit::WebURLResponse;

#ifdef _MSC_VER
// Do not warn about use of std::copy with raw pointers.
#pragma warning(disable : 4996)
#endif

namespace content {

PepperURLLoaderHost::PepperURLLoaderHost(RendererPpapiHostImpl* host,
                                         bool main_document_loader,
                                         PP_Instance instance,
                                         PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      renderer_ppapi_host_(host),
      main_document_loader_(main_document_loader),
      has_universal_access_(false),
      bytes_sent_(0),
      total_bytes_to_be_sent_(-1),
      bytes_received_(0),
      total_bytes_to_be_received_(-1) {
  DCHECK((main_document_loader && !resource) ||
         (!main_document_loader && resource));
}

PepperURLLoaderHost::~PepperURLLoaderHost() {
  // Normally deleting this object will delete the loader which will implicitly
  // cancel the load. But this won't happen for the main document loader. So it
  // would be nice to issue a Close() here.
  //
  // However, the PDF plugin will cancel the document load and then close the
  // resource (which is reasonable). It then makes a second request to load the
  // document so it can set the "want progress" flags (which is unreasonable --
  // we should probably provide download progress on document loads).
  //
  // But a Close() on the main document (even if the request is already
  // canceled) will cancel all pending subresources, of which the second
  // request is one, and the load will fail. Even if we fixed the PDF reader to
  // change the timing or to send progress events to avoid the second request,
  // we don't want to cancel other loads when the main one is closed.
  //
  // "Leaking" the main document load here by not closing it will only affect
  // plugins handling main document loads (which are very few, mostly only PDF)
  // that dereference without explicitly closing the main document load (which
  // PDF doesn't do -- it explicitly closes it before issuing the second
  // request). And the worst thing that will happen is that any remaining data
  // will get queued inside WebKit.
  if (main_document_loader_) {
    // The PluginInstance has a non-owning pointer to us.
    webkit::ppapi::PluginInstanceImpl* instance_object =
        renderer_ppapi_host_->GetPluginInstanceImpl(pp_instance());
    if (instance_object) {
      DCHECK(instance_object->document_loader() == this);
      instance_object->set_document_loader(NULL);
    }
  }

  // There is a path whereby the destructor for the loader_ member can
  // invoke InstanceWasDeleted() upon this URLLoaderResource, thereby
  // re-entering the scoped_ptr destructor with the same scoped_ptr object
  // via loader_.reset(). Be sure that loader_ is first NULL then destroy
  // the scoped_ptr. See http://crbug.com/159429.
  scoped_ptr<WebKit::WebURLLoader> for_destruction_only(loader_.release());

  for (size_t i = 0; i < pending_replies_.size(); i++)
    delete pending_replies_[i];
}

int32_t PepperURLLoaderHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  IPC_BEGIN_MESSAGE_MAP(PepperURLLoaderHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(
        PpapiHostMsg_URLLoader_Open,
        OnHostMsgOpen)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(
        PpapiHostMsg_URLLoader_SetDeferLoading,
        OnHostMsgSetDeferLoading)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
        PpapiHostMsg_URLLoader_Close,
        OnHostMsgClose);
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
        PpapiHostMsg_URLLoader_GrantUniversalAccess,
        OnHostMsgGrantUniversalAccess)
  IPC_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

void PepperURLLoaderHost::willSendRequest(
    WebURLLoader* loader,
    WebURLRequest& new_request,
    const WebURLResponse& redirect_response) {
  if (!request_data_.follow_redirects) {
    SaveResponse(redirect_response);
    SetDefersLoading(true);
  }
}

void PepperURLLoaderHost::didSendData(
    WebURLLoader* loader,
    unsigned long long bytes_sent,
    unsigned long long total_bytes_to_be_sent) {
  // TODO(darin): Bounds check input?
  bytes_sent_ = static_cast<int64_t>(bytes_sent);
  total_bytes_to_be_sent_ = static_cast<int64_t>(total_bytes_to_be_sent);
  UpdateProgress();
}

void PepperURLLoaderHost::didReceiveResponse(WebURLLoader* loader,
                                             const WebURLResponse& response) {
  // Sets -1 if the content length is unknown. Send before issuing callback.
  total_bytes_to_be_received_ = response.expectedContentLength();
  UpdateProgress();

  SaveResponse(response);
}

void PepperURLLoaderHost::didDownloadData(WebURLLoader* loader,
                                          int data_length) {
  bytes_received_ += data_length;
  UpdateProgress();
}

void PepperURLLoaderHost::didReceiveData(WebURLLoader* loader,
                                         const char* data,
                                         int data_length,
                                         int encoded_data_length) {
  // Note that |loader| will be NULL for document loads.
  bytes_received_ += data_length;
  UpdateProgress();

  PpapiPluginMsg_URLLoader_SendData* message =
      new PpapiPluginMsg_URLLoader_SendData;
  message->WriteData(data, data_length);
  SendUpdateToPlugin(message);
}

void PepperURLLoaderHost::didFinishLoading(WebURLLoader* loader,
                                           double finish_time) {
  // Note that |loader| will be NULL for document loads.
  SendUpdateToPlugin(new PpapiPluginMsg_URLLoader_FinishedLoading(PP_OK));
}

void PepperURLLoaderHost::didFail(WebURLLoader* loader,
                                 const WebURLError& error) {
  // Note that |loader| will be NULL for document loads.
  int32_t pp_error = PP_ERROR_FAILED;
  if (error.domain.equals(WebString::fromUTF8(net::kErrorDomain))) {
    // TODO(bbudge): Extend pp_errors.h to cover interesting network errors
    // from the net error domain.
    switch (error.reason) {
      case net::ERR_ACCESS_DENIED:
      case net::ERR_NETWORK_ACCESS_DENIED:
        pp_error = PP_ERROR_NOACCESS;
        break;
    }
  } else {
    // It's a WebKit error.
    pp_error = PP_ERROR_NOACCESS;
  }

  SendUpdateToPlugin(new PpapiPluginMsg_URLLoader_FinishedLoading(pp_error));
}

void PepperURLLoaderHost::DidConnectPendingHostToResource() {
  for (size_t i = 0; i < pending_replies_.size(); i++) {
    host()->SendUnsolicitedReply(pp_resource(), *pending_replies_[i]);
    delete pending_replies_[i];
  }
  pending_replies_.clear();
}

int32_t PepperURLLoaderHost::OnHostMsgOpen(
    ppapi::host::HostMessageContext* context,
    const ppapi::URLRequestInfoData& request_data) {
  // An "Open" isn't a resource Call so has no reply, but failure to open
  // implies a load failure. To make it harder to forget to send the load
  // failed reply from the open handler, we instead catch errors and convert
  // them to load failed messages.
  int32_t ret = InternalOnHostMsgOpen(context, request_data);
  DCHECK(ret != PP_OK_COMPLETIONPENDING);

  if (ret != PP_OK)
    SendUpdateToPlugin(new PpapiPluginMsg_URLLoader_FinishedLoading(ret));
  return PP_OK;
}

// Since this is wrapped by OnHostMsgOpen, we can return errors here and they
// will be translated into a FinishedLoading call automatically.
int32_t PepperURLLoaderHost::InternalOnHostMsgOpen(
    ppapi::host::HostMessageContext* context,
    const ppapi::URLRequestInfoData& request_data) {
  // Main document loads are already open, so don't allow people to open them
  // again.
  if (main_document_loader_)
    return PP_ERROR_INPROGRESS;

  // Create a copy of the request data since CreateWebURLRequest will populate
  // the file refs.
  ppapi::URLRequestInfoData filled_in_request_data = request_data;

  if (webkit::ppapi::URLRequestRequiresUniversalAccess(
          filled_in_request_data) &&
      !has_universal_access_) {
    ppapi::PpapiGlobals::Get()->LogWithSource(
        pp_instance(), PP_LOGLEVEL_ERROR, std::string(),
        "PPB_URLLoader.Open: The URL you're requesting is "
        " on a different security origin than your plugin. To request "
        " cross-origin resources, see "
        " PP_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS.");
    return PP_ERROR_NOACCESS;
  }

  if (loader_.get())
    return PP_ERROR_INPROGRESS;

  WebFrame* frame = GetFrame();
  if (!frame)
    return PP_ERROR_FAILED;
  WebURLRequest web_request;
  if (!webkit::ppapi::CreateWebURLRequest(&filled_in_request_data, frame,
                                          &web_request))
    return PP_ERROR_FAILED;
  web_request.setRequestorProcessID(renderer_ppapi_host_->GetPluginPID());

  WebURLLoaderOptions options;
  if (has_universal_access_) {
    options.allowCredentials = true;
    options.crossOriginRequestPolicy =
        WebURLLoaderOptions::CrossOriginRequestPolicyAllow;
  } else {
    // All other HTTP requests are untrusted.
    options.untrustedHTTP = true;
    if (filled_in_request_data.allow_cross_origin_requests) {
      // Allow cross-origin requests with access control. The request specifies
      // if credentials are to be sent.
      options.allowCredentials = filled_in_request_data.allow_credentials;
      options.crossOriginRequestPolicy =
          WebURLLoaderOptions::CrossOriginRequestPolicyUseAccessControl;
    } else {
      // Same-origin requests can always send credentials.
      options.allowCredentials = true;
    }
  }

  loader_.reset(frame->createAssociatedURLLoader(options));
  if (!loader_.get())
    return PP_ERROR_FAILED;

  // Don't actually save the request until we know we're going to load.
  request_data_ = filled_in_request_data;
  loader_->loadAsynchronously(web_request, this);

  // Although the request is technically pending, this is not a "Call" message
  // so we don't return COMPLETIONPENDING.
  return PP_OK;
}

int32_t PepperURLLoaderHost::OnHostMsgSetDeferLoading(
    ppapi::host::HostMessageContext* context,
    bool defers_loading) {
  SetDefersLoading(defers_loading);
  return PP_OK;
}

int32_t PepperURLLoaderHost::OnHostMsgClose(
    ppapi::host::HostMessageContext* context) {
  Close();
  return PP_OK;
}

int32_t PepperURLLoaderHost::OnHostMsgGrantUniversalAccess(
    ppapi::host::HostMessageContext* context) {
  // Only plugins with private permission can bypass same origin.
  if (!host()->permissions().HasPermission(ppapi::PERMISSION_PRIVATE))
    return PP_ERROR_FAILED;
  has_universal_access_ = true;
  return PP_OK;
}

void PepperURLLoaderHost::SendUpdateToPlugin(IPC::Message* msg) {
  if (pp_resource()) {
    host()->SendUnsolicitedReply(pp_resource(), *msg);
    delete msg;
  } else {
    pending_replies_.push_back(msg);
  }
}

void PepperURLLoaderHost::Close() {
  if (loader_.get())
    loader_->cancel();
  else if (main_document_loader_)
    GetFrame()->stopLoading();
}

WebKit::WebFrame* PepperURLLoaderHost::GetFrame() {
  webkit::ppapi::PluginInstance* instance_object =
      renderer_ppapi_host_->GetPluginInstance(pp_instance());
  if (!instance_object)
    return NULL;
  return instance_object->GetContainer()->element().document().frame();
}

void PepperURLLoaderHost::SetDefersLoading(bool defers_loading) {
  if (loader_.get())
    loader_->setDefersLoading(defers_loading);

  // TODO(brettw) bug 96770: We need a way to set the defers loading flag on
  // main document loads (when the loader_ is null).
}

void PepperURLLoaderHost::SaveResponse(const WebURLResponse& response) {
  if (!main_document_loader_) {
    // When we're the main document loader, we send the response data up front,
    // so we don't want to trigger any callbacks in the plugin which aren't
    // expected. We should not be getting redirects so the response sent
    // up-front should be valid (plugin document loads happen after all
    // redirects are processed since WebKit has to know the MIME type).
    SendUpdateToPlugin(
        new PpapiPluginMsg_URLLoader_ReceivedResponse(
            DataFromWebURLResponse(pp_instance(), response)));
  }
}

void PepperURLLoaderHost::UpdateProgress() {
  bool record_download = request_data_.record_download_progress;
  bool record_upload = request_data_.record_upload_progress;
  if (record_download || record_upload) {
    // Here we go through some effort to only send the exact information that
    // the requestor wanted in the request flags. It would be just as
    // efficient to send all of it, but we don't want people to rely on
    // getting download progress when they happen to set the upload progress
    // flag.
    ppapi::proxy::ResourceMessageReplyParams params;
    SendUpdateToPlugin(new PpapiPluginMsg_URLLoader_UpdateProgress(
            record_upload ? bytes_sent_ : -1,
            record_upload ? total_bytes_to_be_sent_ : -1,
            record_download ? bytes_received_ : -1,
            record_download ? total_bytes_to_be_received_ : -1));
  }
}

}  // namespace content
