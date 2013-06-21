// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_MEDIA_STREAM_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_MEDIA_STREAM_DISPATCHER_HOST_H_

#include <map>
#include <string>
#include <utility>

#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/browser/renderer_host/media/media_stream_requester.h"
#include "content/common/content_export.h"
#include "content/common/media/media_stream_options.h"
#include "content/public/browser/browser_message_filter.h"

namespace content {
class MediaStreamManager;

// MediaStreamDispatcherHost is a delegate for Media Stream API messages used by
// MediaStreamImpl. It's the complement of MediaStreamDispatcher
// (owned by RenderView).
class CONTENT_EXPORT MediaStreamDispatcherHost : public BrowserMessageFilter,
                                                 public MediaStreamRequester {
 public:
  MediaStreamDispatcherHost(int render_process_id,
                            MediaStreamManager* media_stream_manager);

  // MediaStreamRequester implementation.
  virtual void StreamGenerated(
      const std::string& label,
      const StreamDeviceInfoArray& audio_devices,
      const StreamDeviceInfoArray& video_devices) OVERRIDE;
  virtual void StreamGenerationFailed(const std::string& label) OVERRIDE;
  virtual void DevicesEnumerated(const std::string& label,
                                 const StreamDeviceInfoArray& devices) OVERRIDE;
  virtual void DeviceOpened(const std::string& label,
                            const StreamDeviceInfo& video_device) OVERRIDE;

  // BrowserMessageFilter implementation.
  virtual bool OnMessageReceived(const IPC::Message& message,
                                 bool* message_was_ok) OVERRIDE;
  virtual void OnChannelClosing() OVERRIDE;

 protected:
  virtual ~MediaStreamDispatcherHost();

 private:
  friend class MockMediaStreamDispatcherHost;

  void OnGenerateStream(int render_view_id,
                        int page_request_id,
                        const StreamOptions& components,
                        const GURL& security_origin);
  void OnCancelGenerateStream(int render_view_id,
                              int page_request_id);
  void OnStopGeneratedStream(int render_view_id, const std::string& label);

  void OnEnumerateDevices(int render_view_id,
                          int page_request_id,
                          MediaStreamType type,
                          const GURL& security_origin);

  void OnOpenDevice(int render_view_id,
                    int page_request_id,
                    const std::string& device_id,
                    MediaStreamType type,
                    const GURL& security_origin);

  int render_process_id_;
  MediaStreamManager* media_stream_manager_;

  struct StreamRequest;
  typedef std::map<std::string, StreamRequest> StreamMap;
  // Streams generated for this host.
  StreamMap streams_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamDispatcherHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_MEDIA_STREAM_DISPATCHER_HOST_H_
