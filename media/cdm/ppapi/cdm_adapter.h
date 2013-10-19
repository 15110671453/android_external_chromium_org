// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_PPAPI_CDM_ADAPTER_H_
#define MEDIA_CDM_PPAPI_CDM_ADAPTER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "media/cdm/ppapi/api/content_decryption_module.h"
#include "media/cdm/ppapi/cdm_helpers.h"
#include "media/cdm/ppapi/linked_ptr.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/private/pp_content_decryptor.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/private/content_decryptor_private.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/utility/completion_callback_factory.h"

namespace media {

// GetCdmHostFunc implementation.
void* GetCdmHost(int host_interface_version, void* user_data);

// An adapter class for abstracting away PPAPI interaction and threading for a
// Content Decryption Module (CDM).
class CdmAdapter : public pp::Instance,
                   public pp::ContentDecryptor_Private,
                   public cdm::Host {
 public:
  CdmAdapter(PP_Instance instance, pp::Module* module);
  virtual ~CdmAdapter();

  // pp::Instance implementation.
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    return true;
  }

  // PPP_ContentDecryptor_Private implementation.
  // Note: Results of calls to these methods must be reported through the
  // PPB_ContentDecryptor_Private interface.
  virtual void Initialize(const std::string& key_system,
                          bool can_challenge_platform) OVERRIDE;
  virtual void GenerateKeyRequest(const std::string& type,
                                  pp::VarArrayBuffer init_data) OVERRIDE;
  virtual void AddKey(const std::string& session_id,
                      pp::VarArrayBuffer key,
                      pp::VarArrayBuffer init_data) OVERRIDE;
  virtual void CancelKeyRequest(const std::string& session_id) OVERRIDE;
  virtual void Decrypt(
      pp::Buffer_Dev encrypted_buffer,
      const PP_EncryptedBlockInfo& encrypted_block_info) OVERRIDE;
  virtual void InitializeAudioDecoder(
      const PP_AudioDecoderConfig& decoder_config,
      pp::Buffer_Dev extra_data_buffer) OVERRIDE;
  virtual void InitializeVideoDecoder(
      const PP_VideoDecoderConfig& decoder_config,
      pp::Buffer_Dev extra_data_buffer) OVERRIDE;
  virtual void DeinitializeDecoder(PP_DecryptorStreamType decoder_type,
                                   uint32_t request_id) OVERRIDE;
  virtual void ResetDecoder(PP_DecryptorStreamType decoder_type,
                            uint32_t request_id) OVERRIDE;
  virtual void DecryptAndDecode(
      PP_DecryptorStreamType decoder_type,
      pp::Buffer_Dev encrypted_buffer,
      const PP_EncryptedBlockInfo& encrypted_block_info) OVERRIDE;

  // cdm::Host implementation.
  virtual cdm::Buffer* Allocate(int32_t capacity) OVERRIDE;
  virtual void SetTimer(int64_t delay_ms, void* context) OVERRIDE;
  virtual double GetCurrentWallTimeInSeconds() OVERRIDE;
  virtual void SendKeyMessage(
      const char* session_id, int32_t session_id_length,
      const char* message, int32_t message_length,
      const char* default_url, int32_t default_url_length) OVERRIDE;
  virtual void SendKeyError(const char* session_id,
                            int32_t session_id_length,
                            cdm::MediaKeyError error_code,
                            uint32_t system_code) OVERRIDE;
  virtual void GetPrivateData(int32_t* instance,
                              GetPrivateInterface* get_interface) OVERRIDE;

 private:
  struct SessionInfo {
    SessionInfo(const std::string& key_system_in,
                const std::string& session_id_in)
        : key_system(key_system_in),
          session_id(session_id_in) {}
    const std::string key_system;
    const std::string session_id;
  };

  typedef linked_ptr<DecryptedBlockImpl> LinkedDecryptedBlock;
  typedef linked_ptr<VideoFrameImpl> LinkedVideoFrame;
  typedef linked_ptr<AudioFramesImpl> LinkedAudioFrames;

  bool CreateCdmInstance(const std::string& key_system);

  void SendUnknownKeyError(const std::string& key_system,
                           const std::string& session_id);

  void SendKeyAdded(const std::string& key_system,
                    const std::string& session_id);

  // TODO(jrummell): Drop the |key_system| parameter.
  void SendKeyErrorInternal(const std::string& key_system,
                            const std::string& session_id,
                            cdm::MediaKeyError error_code,
                            uint32_t system_code);

  // <code>PPB_ContentDecryptor_Private</code> dispatchers. These are passed to
  // <code>callback_factory_</code> to ensure that calls into
  // <code>PPP_ContentDecryptor_Private</code> are asynchronous.
  void KeyAdded(int32_t result, const SessionInfo& session_info);
  void KeyMessage(int32_t result,
                  const SessionInfo& session_info,
                  const std::vector<uint8>& message,
                  const std::string& default_url);
  void KeyError(int32_t result,
                const SessionInfo& session_info,
                cdm::MediaKeyError error_code,
                uint32_t system_code);
  void DeliverBlock(int32_t result,
                    const cdm::Status& status,
                    const LinkedDecryptedBlock& decrypted_block,
                    const PP_DecryptTrackingInfo& tracking_info);
  void DecoderInitializeDone(int32_t result,
                             PP_DecryptorStreamType decoder_type,
                             uint32_t request_id,
                             bool success);
  void DecoderDeinitializeDone(int32_t result,
                               PP_DecryptorStreamType decoder_type,
                               uint32_t request_id);
  void DecoderResetDone(int32_t result,
                        PP_DecryptorStreamType decoder_type,
                        uint32_t request_id);
  void DeliverFrame(int32_t result,
                    const cdm::Status& status,
                    const LinkedVideoFrame& video_frame,
                    const PP_DecryptTrackingInfo& tracking_info);
  void DeliverSamples(int32_t result,
                      const cdm::Status& status,
                      const LinkedAudioFrames& audio_frames,
                      const PP_DecryptTrackingInfo& tracking_info);

  // Helper for SetTimer().
  void TimerExpired(int32_t result, void* context);

  bool IsValidVideoFrame(const LinkedVideoFrame& video_frame);

  PpbBufferAllocator allocator_;
  pp::CompletionCallbackFactory<CdmAdapter> callback_factory_;
  cdm::ContentDecryptionModule* cdm_;
  std::string key_system_;

  DISALLOW_COPY_AND_ASSIGN(CdmAdapter);
};

}  // namespace media

#endif  // MEDIA_CDM_PPAPI_CDM_ADAPTER_H_
