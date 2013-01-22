// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/ppb_instance_proxy.h"

#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb_audio_config.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/ppb_mouse_lock.h"
#include "ppapi/c/private/pp_content_decryptor.h"
#include "ppapi/proxy/broker_resource.h"
#include "ppapi/proxy/browser_font_singleton_resource.h"
#include "ppapi/proxy/content_decryptor_private_serializer.h"
#include "ppapi/proxy/enter_proxy.h"
#include "ppapi/proxy/flash_clipboard_resource.h"
#include "ppapi/proxy/flash_file_resource.h"
#include "ppapi/proxy/flash_fullscreen_resource.h"
#include "ppapi/proxy/flash_resource.h"
#include "ppapi/proxy/gamepad_resource.h"
#include "ppapi/proxy/host_dispatcher.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_var.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/ppb_url_util_shared.h"
#include "ppapi/shared_impl/ppb_view_shared.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_graphics_2d_api.h"
#include "ppapi/thunk/ppb_graphics_3d_api.h"
#include "ppapi/thunk/thunk.h"

// Windows headers interfere with this file.
#ifdef PostMessage
#undef PostMessage
#endif

using ppapi::thunk::EnterInstanceNoLock;
using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_Graphics2D_API;
using ppapi::thunk::PPB_Graphics3D_API;
using ppapi::thunk::PPB_Instance_API;

namespace ppapi {
namespace proxy {

namespace {

InterfaceProxy* CreateInstanceProxy(Dispatcher* dispatcher) {
  return new PPB_Instance_Proxy(dispatcher);
}

void RequestSurroundingText(PP_Instance instance) {
  PluginDispatcher* dispatcher = PluginDispatcher::GetForInstance(instance);
  if (!dispatcher)
    return;  // Instance has gone away while message was pending.

  InstanceData* data = dispatcher->GetInstanceData(instance);
  DCHECK(data);  // Should have it, since we still have a dispatcher.
  data->is_request_surrounding_text_pending = false;
  if (!data->should_do_request_surrounding_text)
    return;

  // Just fake out a RequestSurroundingText message to the proxy for the PPP
  // interface.
  InterfaceProxy* proxy = dispatcher->GetInterfaceProxy(API_ID_PPP_TEXT_INPUT);
  if (!proxy)
    return;
  proxy->OnMessageReceived(PpapiMsg_PPPTextInput_RequestSurroundingText(
      API_ID_PPP_TEXT_INPUT, instance,
      PPB_Instance_Shared::kExtraCharsForTextInput));
}

}  // namespace

PPB_Instance_Proxy::PPB_Instance_Proxy(Dispatcher* dispatcher)
    : InterfaceProxy(dispatcher),
      callback_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {
}

PPB_Instance_Proxy::~PPB_Instance_Proxy() {
}

// static
const InterfaceProxy::Info* PPB_Instance_Proxy::GetInfoPrivate() {
  static const Info info = {
    ppapi::thunk::GetPPB_Instance_Private_0_1_Thunk(),
    PPB_INSTANCE_PRIVATE_INTERFACE_0_1,
    API_ID_NONE,  // 1_0 is the canonical one.
    false,
    &CreateInstanceProxy,
  };
  return &info;
}

bool PPB_Instance_Proxy::OnMessageReceived(const IPC::Message& msg) {
  // Prevent the dispatcher from going away during a call to ExecuteScript.
  // This must happen OUTSIDE of ExecuteScript since the SerializedVars use
  // the dispatcher upon return of the function (converting the
  // SerializedVarReturnValue/OutParam to a SerializedVar in the destructor).
#if !defined(OS_NACL)
  ScopedModuleReference death_grip(dispatcher());
#endif

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PPB_Instance_Proxy, msg)
#if !defined(OS_NACL)
    // Plugin -> Host messages.
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_GetWindowObject,
                        OnHostMsgGetWindowObject)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_GetOwnerElementObject,
                        OnHostMsgGetOwnerElementObject)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_BindGraphics,
                        OnHostMsgBindGraphics)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_IsFullFrame,
                        OnHostMsgIsFullFrame)
    IPC_MESSAGE_HANDLER(
        PpapiHostMsg_PPBInstance_GetAudioHardwareOutputSampleRate,
        OnHostMsgGetAudioHardwareOutputSampleRate)
    IPC_MESSAGE_HANDLER(
        PpapiHostMsg_PPBInstance_GetAudioHardwareOutputBufferSize,
        OnHostMsgGetAudioHardwareOutputBufferSize)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_ExecuteScript,
                        OnHostMsgExecuteScript)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_GetDefaultCharSet,
                        OnHostMsgGetDefaultCharSet)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_PostMessage,
                        OnHostMsgPostMessage)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_SetFullscreen,
                        OnHostMsgSetFullscreen)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_GetScreenSize,
                        OnHostMsgGetScreenSize)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_RequestInputEvents,
                        OnHostMsgRequestInputEvents)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_ClearInputEvents,
                        OnHostMsgClearInputEvents)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_LockMouse,
                        OnHostMsgLockMouse)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_UnlockMouse,
                        OnHostMsgUnlockMouse)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_SetCursor,
                        OnHostMsgSetCursor)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_SetTextInputType,
                        OnHostMsgSetTextInputType)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_UpdateCaretPosition,
                        OnHostMsgUpdateCaretPosition)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_CancelCompositionText,
                        OnHostMsgCancelCompositionText)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_UpdateSurroundingText,
                        OnHostMsgUpdateSurroundingText)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_GetDocumentURL,
                        OnHostMsgGetDocumentURL)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_ResolveRelativeToDocument,
                        OnHostMsgResolveRelativeToDocument)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_DocumentCanRequest,
                        OnHostMsgDocumentCanRequest)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_DocumentCanAccessDocument,
                        OnHostMsgDocumentCanAccessDocument)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_GetPluginInstanceURL,
                        OnHostMsgGetPluginInstanceURL)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_NeedKey,
                        OnHostMsgNeedKey)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_KeyAdded,
                        OnHostMsgKeyAdded)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_KeyMessage,
                        OnHostMsgKeyMessage)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_KeyError,
                        OnHostMsgKeyError)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_DeliverBlock,
                        OnHostMsgDeliverBlock)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_DecoderInitializeDone,
                        OnHostMsgDecoderInitializeDone)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_DecoderDeinitializeDone,
                        OnHostMsgDecoderDeinitializeDone)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_DecoderResetDone,
                        OnHostMsgDecoderResetDone)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_DeliverFrame,
                        OnHostMsgDeliverFrame)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBInstance_DeliverSamples,
                        OnHostMsgDeliverSamples)
#endif  // !defined(OS_NACL)

    // Host -> Plugin messages.
    IPC_MESSAGE_HANDLER(PpapiMsg_PPBInstance_MouseLockComplete,
                        OnPluginMsgMouseLockComplete)

    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

PP_Bool PPB_Instance_Proxy::BindGraphics(PP_Instance instance,
                                         PP_Resource device) {
  // If device is 0, pass a null HostResource. This signals the host to unbind
  // all devices.
  HostResource host_resource;
  PP_Resource pp_resource = 0;
  if (device) {
    Resource* resource =
        PpapiGlobals::Get()->GetResourceTracker()->GetResource(device);
    if (!resource || resource->pp_instance() != instance)
      return PP_FALSE;
    host_resource = resource->host_resource();
    pp_resource = resource->pp_resource();
  } else {
    // Passing 0 means unbinding all devices.
    dispatcher()->Send(new PpapiHostMsg_PPBInstance_BindGraphics(
        API_ID_PPB_INSTANCE, instance, 0));
    return PP_TRUE;
  }

  // We need to pass different resource to Graphics 2D and 3D right now.  Once
  // 3D is migrated to the new design, we should be able to unify this.
  EnterResourceNoLock<PPB_Graphics2D_API> enter_2d(device, false);
  EnterResourceNoLock<PPB_Graphics3D_API> enter_3d(device, false);
  if (enter_2d.succeeded()) {
    dispatcher()->Send(new PpapiHostMsg_PPBInstance_BindGraphics(
        API_ID_PPB_INSTANCE, instance, pp_resource));
    return PP_TRUE;
  } else if (enter_3d.succeeded()) {
    dispatcher()->Send(new PpapiHostMsg_PPBInstance_BindGraphics(
        API_ID_PPB_INSTANCE, instance, host_resource.host_resource()));
    return PP_TRUE;
  }
  return PP_FALSE;
}

PP_Bool PPB_Instance_Proxy::IsFullFrame(PP_Instance instance) {
  PP_Bool result = PP_FALSE;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_IsFullFrame(
      API_ID_PPB_INSTANCE, instance, &result));
  return result;
}

const ViewData* PPB_Instance_Proxy::GetViewData(PP_Instance instance) {
  InstanceData* data = static_cast<PluginDispatcher*>(dispatcher())->
      GetInstanceData(instance);
  if (!data)
    return NULL;
  return &data->view;
}

PP_Bool PPB_Instance_Proxy::FlashIsFullscreen(PP_Instance instance) {
  // This function is only used for proxying in the renderer process. It is not
  // implemented in the plugin process.
  NOTREACHED();
  return PP_FALSE;
}

PP_Var PPB_Instance_Proxy::GetWindowObject(PP_Instance instance) {
  ReceiveSerializedVarReturnValue result;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_GetWindowObject(
      API_ID_PPB_INSTANCE, instance, &result));
  return result.Return(dispatcher());
}

PP_Var PPB_Instance_Proxy::GetOwnerElementObject(PP_Instance instance) {
  ReceiveSerializedVarReturnValue result;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_GetOwnerElementObject(
      API_ID_PPB_INSTANCE, instance, &result));
  return result.Return(dispatcher());
}

PP_Var PPB_Instance_Proxy::ExecuteScript(PP_Instance instance,
                                         PP_Var script,
                                         PP_Var* exception) {
  ReceiveSerializedException se(dispatcher(), exception);
  if (se.IsThrown())
    return PP_MakeUndefined();

  ReceiveSerializedVarReturnValue result;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_ExecuteScript(
      API_ID_PPB_INSTANCE, instance,
      SerializedVarSendInput(dispatcher(), script), &se, &result));
  return result.Return(dispatcher());
}

uint32_t PPB_Instance_Proxy::GetAudioHardwareOutputSampleRate(
    PP_Instance instance) {
  uint32_t result = PP_AUDIOSAMPLERATE_NONE;
  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_GetAudioHardwareOutputSampleRate(
          API_ID_PPB_INSTANCE, instance, &result));
  return result;
}

uint32_t PPB_Instance_Proxy::GetAudioHardwareOutputBufferSize(
    PP_Instance instance) {
  uint32_t result = 0;
  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_GetAudioHardwareOutputBufferSize(
          API_ID_PPB_INSTANCE, instance, &result));
  return result;
}

PP_Var PPB_Instance_Proxy::GetDefaultCharSet(PP_Instance instance) {
  PluginDispatcher* dispatcher = PluginDispatcher::GetForInstance(instance);
  if (!dispatcher)
    return PP_MakeUndefined();

  ReceiveSerializedVarReturnValue result;
  dispatcher->Send(new PpapiHostMsg_PPBInstance_GetDefaultCharSet(
      API_ID_PPB_INSTANCE, instance, &result));
  return result.Return(dispatcher);
}

void PPB_Instance_Proxy::NumberOfFindResultsChanged(PP_Instance instance,
                                                    int32_t total,
                                                    PP_Bool final_result) {
  NOTIMPLEMENTED();  // Not proxied yet.
}

void PPB_Instance_Proxy::SelectedFindResultChanged(PP_Instance instance,
                                                   int32_t index) {
  NOTIMPLEMENTED();  // Not proxied yet.
}

PP_Bool PPB_Instance_Proxy::SetFullscreen(PP_Instance instance,
                                          PP_Bool fullscreen) {
  PP_Bool result = PP_FALSE;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_SetFullscreen(
      API_ID_PPB_INSTANCE, instance, fullscreen, &result));
  return result;
}

PP_Bool PPB_Instance_Proxy::GetScreenSize(PP_Instance instance,
                                          PP_Size* size) {
  PP_Bool result = PP_FALSE;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_GetScreenSize(
      API_ID_PPB_INSTANCE, instance, &result, size));
  return result;
}

Resource* PPB_Instance_Proxy::GetSingletonResource(PP_Instance instance,
                                                   SingletonResourceID id) {
  InstanceData* data = static_cast<PluginDispatcher*>(dispatcher())->
      GetInstanceData(instance);

  InstanceData::SingletonResourceMap::iterator it =
      data->singleton_resources.find(id);
  if (it != data->singleton_resources.end())
    return it->second.get();

  scoped_refptr<Resource> new_singleton;
  Connection connection(PluginGlobals::Get()->GetBrowserSender(), dispatcher());

  switch (id) {
    case BROKER_SINGLETON_ID:
      new_singleton = new BrokerResource(connection, instance);
      break;
    case GAMEPAD_SINGLETON_ID:
      new_singleton = new GamepadResource(connection, instance);
      break;
// Flash/trusted resources aren't needed for NaCl.
#if !defined(OS_NACL) && !defined(NACL_WIN64)
    case BROWSER_FONT_SINGLETON_ID:
      new_singleton = new BrowserFontSingletonResource(connection, instance);
      break;
    case FLASH_CLIPBOARD_SINGLETON_ID:
      new_singleton = new FlashClipboardResource(connection, instance);
      break;
    case FLASH_FILE_SINGLETON_ID:
      new_singleton = new FlashFileResource(connection, instance);
      break;
    case FLASH_FULLSCREEN_SINGLETON_ID:
      new_singleton = new FlashFullscreenResource(connection, instance);
      break;
    case FLASH_SINGLETON_ID:
      new_singleton = new FlashResource(connection, instance,
          static_cast<PluginDispatcher*>(dispatcher()));
      break;
#else
    case BROWSER_FONT_SINGLETON_ID:
    case FLASH_CLIPBOARD_SINGLETON_ID:
    case FLASH_FILE_SINGLETON_ID:
    case FLASH_FULLSCREEN_SINGLETON_ID:
    case FLASH_SINGLETON_ID:
      NOTREACHED();
      break;
#endif  // !defined(OS_NACL) && !defined(NACL_WIN64)
  }

  if (!new_singleton) {
    // Getting here implies that a constructor is missing in the above switch.
    NOTREACHED();
    return NULL;
  }

  data->singleton_resources[id] = new_singleton;
  return new_singleton.get();
}

int32_t PPB_Instance_Proxy::RequestInputEvents(PP_Instance instance,
                                               uint32_t event_classes) {
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_RequestInputEvents(
      API_ID_PPB_INSTANCE, instance, false, event_classes));

  // We always register for the classes we can handle, this function validates
  // the flags so we can notify it if anything was invalid, without requiring
  // a sync reply.
  return ValidateRequestInputEvents(false, event_classes);
}

int32_t PPB_Instance_Proxy::RequestFilteringInputEvents(
    PP_Instance instance,
    uint32_t event_classes) {
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_RequestInputEvents(
      API_ID_PPB_INSTANCE, instance, true, event_classes));

  // We always register for the classes we can handle, this function validates
  // the flags so we can notify it if anything was invalid, without requiring
  // a sync reply.
  return ValidateRequestInputEvents(true, event_classes);
}

void PPB_Instance_Proxy::ClearInputEventRequest(PP_Instance instance,
                                                uint32_t event_classes) {
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_ClearInputEvents(
      API_ID_PPB_INSTANCE, instance, event_classes));
}

void PPB_Instance_Proxy::ZoomChanged(PP_Instance instance,
                                     double factor) {
  // Not proxied yet.
  NOTIMPLEMENTED();
}

void PPB_Instance_Proxy::ZoomLimitsChanged(PP_Instance instance,
                                           double minimum_factor,
                                           double maximium_factor) {
  // Not proxied yet.
  NOTIMPLEMENTED();
}

PP_Var PPB_Instance_Proxy::GetDocumentURL(PP_Instance instance,
                                          PP_URLComponents_Dev* components) {
  ReceiveSerializedVarReturnValue result;
  PP_URLComponents_Dev url_components;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_GetDocumentURL(
      API_ID_PPB_INSTANCE, instance, &url_components, &result));
  if (components)
    *components = url_components;
  return result.Return(dispatcher());
}

#if !defined(OS_NACL)
PP_Var PPB_Instance_Proxy::ResolveRelativeToDocument(
    PP_Instance instance,
    PP_Var relative,
    PP_URLComponents_Dev* components) {
  ReceiveSerializedVarReturnValue result;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_ResolveRelativeToDocument(
      API_ID_PPB_INSTANCE, instance,
      SerializedVarSendInput(dispatcher(), relative),
      &result));
  return PPB_URLUtil_Shared::ConvertComponentsAndReturnURL(
      result.Return(dispatcher()),
      components);
}

PP_Bool PPB_Instance_Proxy::DocumentCanRequest(PP_Instance instance,
                                               PP_Var url) {
  PP_Bool result = PP_FALSE;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_DocumentCanRequest(
      API_ID_PPB_INSTANCE, instance,
      SerializedVarSendInput(dispatcher(), url),
      &result));
  return result;
}

PP_Bool PPB_Instance_Proxy::DocumentCanAccessDocument(PP_Instance instance,
                                                      PP_Instance target) {
  PP_Bool result = PP_FALSE;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_DocumentCanAccessDocument(
      API_ID_PPB_INSTANCE, instance, target, &result));
  return result;
}

PP_Var PPB_Instance_Proxy::GetPluginInstanceURL(
      PP_Instance instance,
      PP_URLComponents_Dev* components) {
  ReceiveSerializedVarReturnValue result;
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_GetPluginInstanceURL(
      API_ID_PPB_INSTANCE, instance, &result));
  return PPB_URLUtil_Shared::ConvertComponentsAndReturnURL(
      result.Return(dispatcher()),
      components);
}

void PPB_Instance_Proxy::NeedKey(PP_Instance instance,
                                 PP_Var key_system,
                                 PP_Var session_id,
                                 PP_Var init_data) {
  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_NeedKey(
          API_ID_PPB_INSTANCE,
          instance,
          SerializedVarSendInput(dispatcher(), key_system),
          SerializedVarSendInput(dispatcher(), session_id),
          SerializedVarSendInput(dispatcher(), init_data)));
}

void PPB_Instance_Proxy::KeyAdded(PP_Instance instance,
                                  PP_Var key_system,
                                  PP_Var session_id) {
  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_KeyAdded(
          API_ID_PPB_INSTANCE,
          instance,
          SerializedVarSendInput(dispatcher(), key_system),
          SerializedVarSendInput(dispatcher(), session_id)));
}

void PPB_Instance_Proxy::KeyMessage(PP_Instance instance,
                                    PP_Var key_system,
                                    PP_Var session_id,
                                    PP_Var message,
                                    PP_Var default_url) {
  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_KeyMessage(
          API_ID_PPB_INSTANCE,
          instance,
          SerializedVarSendInput(dispatcher(), key_system),
          SerializedVarSendInput(dispatcher(), session_id),
          SerializedVarSendInput(dispatcher(), message),
          SerializedVarSendInput(dispatcher(), default_url)));
}

void PPB_Instance_Proxy::KeyError(PP_Instance instance,
                                  PP_Var key_system,
                                  PP_Var session_id,
                                  int32_t media_error,
                                  int32_t system_code) {
  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_KeyError(
          API_ID_PPB_INSTANCE,
          instance,
          SerializedVarSendInput(dispatcher(), key_system),
          SerializedVarSendInput(dispatcher(), session_id),
          media_error,
          system_code));
}

void PPB_Instance_Proxy::DeliverBlock(PP_Instance instance,
                                      PP_Resource decrypted_block,
                                      const PP_DecryptedBlockInfo* block_info) {
  PP_Resource decrypted_block_host_resource = 0;

  if (decrypted_block) {
    Resource* object =
        PpapiGlobals::Get()->GetResourceTracker()->GetResource(decrypted_block);
    if (!object || object->pp_instance() != instance) {
      NOTREACHED();
      return;
    }
    decrypted_block_host_resource = object->host_resource().host_resource();
  }

  std::string serialized_block_info;
  if (!SerializeBlockInfo(*block_info, &serialized_block_info)) {
    NOTREACHED();
    return;
  }

  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_DeliverBlock(API_ID_PPB_INSTANCE,
          instance,
          decrypted_block_host_resource,
          serialized_block_info));
}

void PPB_Instance_Proxy::DecoderInitializeDone(
    PP_Instance instance,
    PP_DecryptorStreamType decoder_type,
    uint32_t request_id,
    PP_Bool success) {
  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_DecoderInitializeDone(
          API_ID_PPB_INSTANCE,
          instance,
          decoder_type,
          request_id,
          success));
}

void PPB_Instance_Proxy::DecoderDeinitializeDone(
    PP_Instance instance,
    PP_DecryptorStreamType decoder_type,
    uint32_t request_id) {
  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_DecoderDeinitializeDone(
          API_ID_PPB_INSTANCE,
          instance,
          decoder_type,
          request_id));
}

void PPB_Instance_Proxy::DecoderResetDone(PP_Instance instance,
                                          PP_DecryptorStreamType decoder_type,
                                          uint32_t request_id) {
  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_DecoderResetDone(
          API_ID_PPB_INSTANCE,
          instance,
          decoder_type,
          request_id));
}

void PPB_Instance_Proxy::DeliverFrame(PP_Instance instance,
                                      PP_Resource decrypted_frame,
                                      const PP_DecryptedFrameInfo* frame_info) {
  PP_Resource host_resource = 0;
  if (decrypted_frame != 0) {
    ResourceTracker* tracker = PpapiGlobals::Get()->GetResourceTracker();
    Resource* object = tracker->GetResource(decrypted_frame);

    if (!object || object->pp_instance() != instance) {
      NOTREACHED();
      return;
    }

    host_resource = object->host_resource().host_resource();
  }

  std::string serialized_frame_info;
  if (!SerializeBlockInfo(*frame_info, &serialized_frame_info)) {
    NOTREACHED();
    return;
  }

  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_DeliverFrame(API_ID_PPB_INSTANCE,
                                                instance,
                                                host_resource,
                                                serialized_frame_info));
}

void PPB_Instance_Proxy::DeliverSamples(
    PP_Instance instance,
    PP_Resource decrypted_samples,
    const PP_DecryptedBlockInfo* block_info) {
  PP_Resource host_resource = 0;
  if (decrypted_samples != 0) {
    ResourceTracker* tracker = PpapiGlobals::Get()->GetResourceTracker();
    Resource* object = tracker->GetResource(decrypted_samples);

    if (!object || object->pp_instance() != instance) {
      NOTREACHED();
      return;
    }

    host_resource = object->host_resource().host_resource();
  }

  std::string serialized_block_info;
  if (!SerializeBlockInfo(*block_info, &serialized_block_info)) {
    NOTREACHED();
    return;
  }

  dispatcher()->Send(
      new PpapiHostMsg_PPBInstance_DeliverSamples(API_ID_PPB_INSTANCE,
                                                  instance,
                                                  host_resource,
                                                  serialized_block_info));
}
#endif  // !defined(OS_NACL)

void PPB_Instance_Proxy::PostMessage(PP_Instance instance,
                                     PP_Var message) {
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_PostMessage(
      API_ID_PPB_INSTANCE,
      instance, SerializedVarSendInput(dispatcher(), message)));
}

PP_Bool PPB_Instance_Proxy::SetCursor(PP_Instance instance,
                                      PP_MouseCursor_Type type,
                                      PP_Resource image,
                                      const PP_Point* hot_spot) {
  // Some of these parameters are important for security. This check is in the
  // plugin process just for the convenience of the caller (since we don't
  // bother returning errors from the other process with a sync message). The
  // parameters will be validated again in the renderer.
  if (!ValidateSetCursorParams(type, image, hot_spot))
    return PP_FALSE;

  HostResource image_host_resource;
  if (image) {
    Resource* cursor_image =
        PpapiGlobals::Get()->GetResourceTracker()->GetResource(image);
    if (!cursor_image || cursor_image->pp_instance() != instance)
      return PP_FALSE;
    image_host_resource = cursor_image->host_resource();
  }

  dispatcher()->Send(new PpapiHostMsg_PPBInstance_SetCursor(
      API_ID_PPB_INSTANCE, instance, static_cast<int32_t>(type),
      image_host_resource, hot_spot ? *hot_spot : PP_MakePoint(0, 0)));
  return PP_TRUE;
}

int32_t PPB_Instance_Proxy::LockMouse(PP_Instance instance,
                                      scoped_refptr<TrackedCallback> callback) {
  // Save the mouse callback on the instance data.
  InstanceData* data = static_cast<PluginDispatcher*>(dispatcher())->
      GetInstanceData(instance);
  if (!data)
    return PP_ERROR_BADARGUMENT;
  if (TrackedCallback::IsPending(data->mouse_lock_callback))
    return PP_ERROR_INPROGRESS;  // Already have a pending callback.
  data->mouse_lock_callback = callback;

  dispatcher()->Send(new PpapiHostMsg_PPBInstance_LockMouse(
      API_ID_PPB_INSTANCE, instance));
  return PP_OK_COMPLETIONPENDING;
}

void PPB_Instance_Proxy::UnlockMouse(PP_Instance instance) {
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_UnlockMouse(
      API_ID_PPB_INSTANCE, instance));
}

void PPB_Instance_Proxy::SetTextInputType(PP_Instance instance,
                                          PP_TextInput_Type type) {
  CancelAnyPendingRequestSurroundingText(instance);
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_SetTextInputType(
      API_ID_PPB_INSTANCE, instance, type));
}

void PPB_Instance_Proxy::UpdateCaretPosition(PP_Instance instance,
                                             const PP_Rect& caret,
                                             const PP_Rect& bounding_box) {
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_UpdateCaretPosition(
      API_ID_PPB_INSTANCE, instance, caret, bounding_box));
}

void PPB_Instance_Proxy::CancelCompositionText(PP_Instance instance) {
  CancelAnyPendingRequestSurroundingText(instance);
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_CancelCompositionText(
      API_ID_PPB_INSTANCE, instance));
}

void PPB_Instance_Proxy::SelectionChanged(PP_Instance instance) {
  // The "right" way to do this is to send the message to the host. However,
  // all it will do is call RequestSurroundingText with a hardcoded number of
  // characters in response, which is an entire IPC round-trip.
  //
  // We can avoid this round-trip by just implementing the
  // RequestSurroundingText logic in the plugin process. If the logic in the
  // host becomes more complex (like a more adaptive number of characters),
  // we'll need to reevanuate whether we want to do the round trip instead.
  //
  // Be careful to post a task to avoid reentering the plugin.

  InstanceData* data =
      static_cast<PluginDispatcher*>(dispatcher())->GetInstanceData(instance);
  if (!data)
    return;
  data->should_do_request_surrounding_text = true;

  if (!data->is_request_surrounding_text_pending) {
    MessageLoop::current()->PostTask(
        FROM_HERE,
        RunWhileLocked(base::Bind(&RequestSurroundingText, instance)));
    data->is_request_surrounding_text_pending = true;
  }
}

void PPB_Instance_Proxy::UpdateSurroundingText(PP_Instance instance,
                                               const char* text,
                                               uint32_t caret,
                                               uint32_t anchor) {
  dispatcher()->Send(new PpapiHostMsg_PPBInstance_UpdateSurroundingText(
      API_ID_PPB_INSTANCE, instance, text, caret, anchor));
}

#if !defined(OS_NACL)
void PPB_Instance_Proxy::OnHostMsgGetWindowObject(
    PP_Instance instance,
    SerializedVarReturnValue result) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    result.Return(dispatcher(), enter.functions()->GetWindowObject(instance));
}

void PPB_Instance_Proxy::OnHostMsgGetOwnerElementObject(
    PP_Instance instance,
    SerializedVarReturnValue result) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    result.Return(dispatcher(),
                  enter.functions()->GetOwnerElementObject(instance));
  }
}

void PPB_Instance_Proxy::OnHostMsgBindGraphics(PP_Instance instance,
                                               PP_Resource device) {
  // Note that we ignroe the return value here. Otherwise, this would need to
  // be a slow sync call, and the plugin side of the proxy will have already
  // validated the resources, so we shouldn't see errors here that weren't
  // already caught.
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->BindGraphics(instance, device);
}

void PPB_Instance_Proxy::OnHostMsgGetAudioHardwareOutputSampleRate(
    PP_Instance instance, uint32_t* result) {
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    *result = enter.functions()->GetAudioHardwareOutputSampleRate(instance);
}

void PPB_Instance_Proxy::OnHostMsgGetAudioHardwareOutputBufferSize(
    PP_Instance instance, uint32_t* result) {
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    *result = enter.functions()->GetAudioHardwareOutputBufferSize(instance);
}

void PPB_Instance_Proxy::OnHostMsgIsFullFrame(PP_Instance instance,
                                              PP_Bool* result) {
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    *result = enter.functions()->IsFullFrame(instance);
}

void PPB_Instance_Proxy::OnHostMsgExecuteScript(
    PP_Instance instance,
    SerializedVarReceiveInput script,
    SerializedVarOutParam out_exception,
    SerializedVarReturnValue result) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.failed())
    return;

  if (dispatcher()->IsPlugin())
    NOTREACHED();
  else
    static_cast<HostDispatcher*>(dispatcher())->set_allow_plugin_reentrancy();

  result.Return(dispatcher(), enter.functions()->ExecuteScript(
      instance,
      script.Get(dispatcher()),
      out_exception.OutParam(dispatcher())));
}

void PPB_Instance_Proxy::OnHostMsgGetDefaultCharSet(
    PP_Instance instance,
    SerializedVarReturnValue result) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    result.Return(dispatcher(), enter.functions()->GetDefaultCharSet(instance));
}

void PPB_Instance_Proxy::OnHostMsgSetFullscreen(PP_Instance instance,
                                                PP_Bool fullscreen,
                                                PP_Bool* result) {
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    *result = enter.functions()->SetFullscreen(instance, fullscreen);
}


void PPB_Instance_Proxy::OnHostMsgGetScreenSize(PP_Instance instance,
                                                PP_Bool* result,
                                                PP_Size* size) {
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    *result = enter.functions()->GetScreenSize(instance, size);
}

void PPB_Instance_Proxy::OnHostMsgRequestInputEvents(PP_Instance instance,
                                                     bool is_filtering,
                                                     uint32_t event_classes) {
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    if (is_filtering)
      enter.functions()->RequestFilteringInputEvents(instance, event_classes);
    else
      enter.functions()->RequestInputEvents(instance, event_classes);
  }
}

void PPB_Instance_Proxy::OnHostMsgClearInputEvents(PP_Instance instance,
                                                   uint32_t event_classes) {
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->ClearInputEventRequest(instance, event_classes);
}

void PPB_Instance_Proxy::OnHostMsgPostMessage(
    PP_Instance instance,
    SerializedVarReceiveInput message) {
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->PostMessage(instance, message.Get(dispatcher()));
}

void PPB_Instance_Proxy::OnHostMsgLockMouse(PP_Instance instance) {
  // Need to be careful to always issue the callback.
  pp::CompletionCallback cb = callback_factory_.NewCallback(
      &PPB_Instance_Proxy::MouseLockCompleteInHost, instance);

  EnterInstanceNoLock enter(instance, cb.pp_completion_callback());
  if (enter.succeeded())
    enter.SetResult(enter.functions()->LockMouse(instance, enter.callback()));
}

void PPB_Instance_Proxy::OnHostMsgUnlockMouse(PP_Instance instance) {
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->UnlockMouse(instance);
}

void PPB_Instance_Proxy::OnHostMsgGetDocumentURL(
    PP_Instance instance,
    PP_URLComponents_Dev* components,
    SerializedVarReturnValue result) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    PP_Var document_url = enter.functions()->GetDocumentURL(instance,
                                                            components);
    result.Return(dispatcher(), document_url);
  }
}

void PPB_Instance_Proxy::OnHostMsgResolveRelativeToDocument(
    PP_Instance instance,
    SerializedVarReceiveInput relative,
    SerializedVarReturnValue result) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    result.Return(dispatcher(),
                  enter.functions()->ResolveRelativeToDocument(
                      instance, relative.Get(dispatcher()), NULL));
  }
}

void PPB_Instance_Proxy::OnHostMsgDocumentCanRequest(
    PP_Instance instance,
    SerializedVarReceiveInput url,
    PP_Bool* result) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    *result = enter.functions()->DocumentCanRequest(instance,
                                                    url.Get(dispatcher()));
  }
}

void PPB_Instance_Proxy::OnHostMsgDocumentCanAccessDocument(PP_Instance active,
                                                            PP_Instance target,
                                                            PP_Bool* result) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  EnterInstanceNoLock enter(active);
  if (enter.succeeded())
    *result = enter.functions()->DocumentCanAccessDocument(active, target);
}

void PPB_Instance_Proxy::OnHostMsgGetPluginInstanceURL(
    PP_Instance instance,
    SerializedVarReturnValue result) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    result.Return(dispatcher(),
                  enter.functions()->GetPluginInstanceURL(instance, NULL));
  }
}

void PPB_Instance_Proxy::OnHostMsgNeedKey(PP_Instance instance,
                                          SerializedVarReceiveInput key_system,
                                          SerializedVarReceiveInput session_id,
                                          SerializedVarReceiveInput init_data) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    enter.functions()->NeedKey(instance,
                               key_system.Get(dispatcher()),
                               session_id.Get(dispatcher()),
                               init_data.Get(dispatcher()));
  }
}

void PPB_Instance_Proxy::OnHostMsgKeyAdded(
    PP_Instance instance,
    SerializedVarReceiveInput key_system,
    SerializedVarReceiveInput session_id) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    enter.functions()->KeyAdded(instance,
                                key_system.Get(dispatcher()),
                                session_id.Get(dispatcher()));
  }
}

void PPB_Instance_Proxy::OnHostMsgKeyMessage(
    PP_Instance instance,
    SerializedVarReceiveInput key_system,
    SerializedVarReceiveInput session_id,
    SerializedVarReceiveInput message,
    SerializedVarReceiveInput default_url) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    enter.functions()->KeyMessage(instance,
                                  key_system.Get(dispatcher()),
                                  session_id.Get(dispatcher()),
                                  message.Get(dispatcher()),
                                  default_url.Get(dispatcher()));
  }
}

void PPB_Instance_Proxy::OnHostMsgKeyError(
    PP_Instance instance,
    SerializedVarReceiveInput key_system,
    SerializedVarReceiveInput session_id,
    int32_t media_error,
    int32_t system_error) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    enter.functions()->KeyError(instance,
                                key_system.Get(dispatcher()),
                                session_id.Get(dispatcher()),
                                media_error,
                                system_error);
  }
}

void PPB_Instance_Proxy::OnHostMsgDeliverBlock(
    PP_Instance instance,
    PP_Resource decrypted_block,
    const std::string& serialized_block_info) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  PP_DecryptedBlockInfo block_info;
  if (!DeserializeBlockInfo(serialized_block_info, &block_info))
    return;

  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->DeliverBlock(instance, decrypted_block, &block_info);
}

void PPB_Instance_Proxy::OnHostMsgDecoderInitializeDone(
    PP_Instance instance,
    PP_DecryptorStreamType decoder_type,
    uint32_t request_id,
    PP_Bool success) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    enter.functions()->DecoderInitializeDone(instance,
                                             decoder_type,
                                             request_id,
                                             success);
  }
}

void PPB_Instance_Proxy::OnHostMsgDecoderDeinitializeDone(
    PP_Instance instance,
    PP_DecryptorStreamType decoder_type,
    uint32_t request_id) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->DecoderDeinitializeDone(instance,
                                               decoder_type,
                                               request_id);
}

void PPB_Instance_Proxy::OnHostMsgDecoderResetDone(
    PP_Instance instance,
    PP_DecryptorStreamType decoder_type,
    uint32_t request_id) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->DecoderResetDone(instance, decoder_type, request_id);
}

void PPB_Instance_Proxy::OnHostMsgDeliverFrame(
    PP_Instance instance,
    PP_Resource decrypted_frame,
    const std::string& serialized_frame_info) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  PP_DecryptedFrameInfo frame_info;
  if (!DeserializeBlockInfo(serialized_frame_info, &frame_info))
    return;

  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->DeliverFrame(instance, decrypted_frame, &frame_info);
}

void PPB_Instance_Proxy::OnHostMsgDeliverSamples(
    PP_Instance instance,
    PP_Resource audio_frames,
    const std::string& serialized_block_info) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_PRIVATE))
    return;
  PP_DecryptedBlockInfo block_info;
  if (!DeserializeBlockInfo(serialized_block_info, &block_info))
    return;

  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->DeliverSamples(instance, audio_frames, &block_info);
}

void  PPB_Instance_Proxy::OnHostMsgSetCursor(
    PP_Instance instance,
    int32_t type,
    const ppapi::HostResource& custom_image,
    const PP_Point& hot_spot) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    enter.functions()->SetCursor(
        instance, static_cast<PP_MouseCursor_Type>(type),
        custom_image.host_resource(), &hot_spot);
  }
}

void PPB_Instance_Proxy::OnHostMsgSetTextInputType(PP_Instance instance,
                                                   PP_TextInput_Type type) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->SetTextInputType(instance, type);
}

void PPB_Instance_Proxy::OnHostMsgUpdateCaretPosition(
    PP_Instance instance,
    const PP_Rect& caret,
    const PP_Rect& bounding_box) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded())
    enter.functions()->UpdateCaretPosition(instance, caret, bounding_box);
}

void PPB_Instance_Proxy::OnHostMsgCancelCompositionText(PP_Instance instance) {
  EnterInstanceNoLock enter(instance);
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  if (enter.succeeded())
    enter.functions()->CancelCompositionText(instance);
}

void PPB_Instance_Proxy::OnHostMsgUpdateSurroundingText(
    PP_Instance instance,
    const std::string& text,
    uint32_t caret,
    uint32_t anchor) {
  if (!dispatcher()->permissions().HasPermission(PERMISSION_DEV))
    return;
  EnterInstanceNoLock enter(instance);
  if (enter.succeeded()) {
    enter.functions()->UpdateSurroundingText(instance, text.c_str(), caret,
                                             anchor);
  }
}
#endif  // !defined(OS_NACL)

void PPB_Instance_Proxy::OnPluginMsgMouseLockComplete(PP_Instance instance,
                                                      int32_t result) {
  if (!dispatcher()->IsPlugin())
    return;

  // Save the mouse callback on the instance data.
  InstanceData* data = static_cast<PluginDispatcher*>(dispatcher())->
      GetInstanceData(instance);
  if (!data)
    return;  // Instance was probably deleted.
  if (!TrackedCallback::IsPending(data->mouse_lock_callback)) {
    NOTREACHED();
    return;
  }
  data->mouse_lock_callback->Run(result);
}

#if !defined(OS_NACL)
void PPB_Instance_Proxy::MouseLockCompleteInHost(int32_t result,
                                                 PP_Instance instance) {
  dispatcher()->Send(new PpapiMsg_PPBInstance_MouseLockComplete(
      API_ID_PPB_INSTANCE, instance, result));
}
#endif  // !defined(OS_NACL)

void PPB_Instance_Proxy::CancelAnyPendingRequestSurroundingText(
    PP_Instance instance) {
  InstanceData* data = static_cast<PluginDispatcher*>(dispatcher())->
      GetInstanceData(instance);
  if (!data)
    return;  // Instance was probably deleted.
  data->should_do_request_surrounding_text = false;
}

}  // namespace proxy
}  // namespace ppapi
