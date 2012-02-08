// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_X509_USER_CERT_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_RENDERER_HOST_X509_USER_CERT_RESOURCE_HANDLER_H_
#pragma once

#include <string>

#include "base/memory/scoped_ptr.h"
#include "content/browser/download/download_buffer.h"
#include "content/browser/renderer_host/resource_handler.h"
#include "googleurl/src/gurl.h"

class ResourceDispatcherHost;

namespace net {
class IOBuffer;
class URLRequest;
class URLRequestStatus;
}  // namespace net

// This class handles the "application/x-x509-user-cert" mime-type
// which is a certificate generated by a CA, typically after a previous
// <keygen> form post.

class X509UserCertResourceHandler : public ResourceHandler {
 public:
  X509UserCertResourceHandler(ResourceDispatcherHost* host,
                              net::URLRequest* request,
                              int render_process_host_id,
                              int render_view_id);

  virtual bool OnUploadProgress(int request_id,
                                uint64 position,
                                uint64 size) OVERRIDE;

  // Not needed, as this event handler ought to be the final resource.
  virtual bool OnRequestRedirected(int request_id,
                                   const GURL& url,
                                   content::ResourceResponse* resp,
                                   bool* defer) OVERRIDE;

  // Check if this indeed an X509 cert.
  virtual bool OnResponseStarted(int request_id,
                                 content::ResourceResponse* resp) OVERRIDE;

  // Pass-through implementation.
  virtual bool OnWillStart(int request_id,
                           const GURL& url,
                           bool* defer) OVERRIDE;

  // Create a new buffer to store received data.
  virtual bool OnWillRead(int request_id,
                          net::IOBuffer** buf,
                          int* buf_size,
                          int min_size) OVERRIDE;

  // A read was completed, maybe allocate a new buffer for further data.
  virtual bool OnReadCompleted(int request_id,
                               int* bytes_read) OVERRIDE;

  // Done downloading the certificate.
  virtual bool OnResponseCompleted(int request_id,
                                   const net::URLRequestStatus& urs,
                                   const std::string& sec_info) OVERRIDE;

  virtual void OnRequestClosed() OVERRIDE;

 private:
  virtual ~X509UserCertResourceHandler();

  void AssembleResource();

  GURL url_;
  ResourceDispatcherHost* host_;
  net::URLRequest* request_;
  size_t content_length_;
  content::ContentVector buffer_;
  scoped_refptr<net::IOBuffer> read_buffer_;
  scoped_refptr<net::IOBuffer> resource_buffer_;  // Downloaded certificate.
  // The id of the |RenderProcessHost| which started the download.
  int render_process_host_id_;
  // The id of the |RenderView| which started the download.
  int render_view_id_;

  DISALLOW_COPY_AND_ASSIGN(X509UserCertResourceHandler);
};

#endif  // CONTENT_BROWSER_RENDERER_HOST_X509_USER_CERT_RESOURCE_HANDLER_H_
