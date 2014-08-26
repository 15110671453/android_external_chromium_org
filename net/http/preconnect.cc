// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Copyright (c) 2012, 2013 The Linux Foundation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/preconnect.h"
#include "base/logging.h"
#include "net/proxy/proxy_info.h"
#include "net/http/http_stream_factory.h"
#include "net/http/http_network_session.h"
#include "net/stat_hub/stat_hub_api.h"
#include "net/stat_hub/stat_hub_cmd_api.h"

namespace net {
// static
void Preconnect::DoPreconnect(HttpNetworkSession* session,
    const GURL& url, int count,
    HttpRequestInfo::RequestMotivation motivation ) {
  Preconnect* preconnect = new Preconnect(session);
  preconnect->Connect(url, count, motivation);
}

Preconnect::Preconnect(HttpNetworkSession* session)
  : session_(session)
{
  io_callback_ = base::Bind(&Preconnect::OnPreconnectComplete, this);
}

Preconnect::~Preconnect() {}

void Preconnect::Connect(const GURL& url, int count,
    HttpRequestInfo::RequestMotivation motivation) {

  request_info_.reset(new HttpRequestInfo());
  request_info_->url = url;
  request_info_->method = "GET";
  request_info_->motivation = motivation;

  // Setup the SSL Configuration.
  ssl_config_.reset(new SSLConfig());
  session_->ssl_config_service()->GetSSLConfig(ssl_config_.get());
  session_->GetNextProtos(&ssl_config_->next_protos);

  // All preconnects should perform EV certificate verification.
  ssl_config_->verify_ev_cert = true;

  proxy_info_.reset(new ProxyInfo());
  HttpStreamFactory* stream_factory = session_->http_stream_factory();

  StatHubCmd* cmd = STAT_HUB_API(CmdCreate)(SH_CMD_GP_EVENT, SH_ACTION_NONE, 0);
  if (NULL!=cmd) {
      cmd->AddParamAsString("Preconnect: ");
      cmd->AddParamAsString(url.spec().c_str());
      STAT_HUB_API(CmdCommit)(cmd);
  }

  // It almost doesn't matter whether we use net::LOWEST or net::HIGHEST
  // priority here, as we won't make a request, and will surrender the created
  // socket to the pool as soon as we can.  However, we would like to mark the
  // speculative socket as such, and IF we use a net::LOWEST priority, and if
  // a navigation asked for a socket (after us) then it would get our socket,
  // and we'd get its later-arriving socket, which might make us record that
  // the speculation didn't help :-/.  By using net::HIGHEST, we ensure that
  // a socket is given to us if "we asked first" and this allows us to mark it
  // as speculative, and better detect stats (if it gets used).
  // TODO(jar): histogram to see how often we accidentally use a previously-
  // unused socket, when a previously used socket was available.
  stream_factory->PreconnectStreams(count, (*request_info_.get()),
    HIGHEST, (*ssl_config_.get()), (*ssl_config_.get()));

  // TODO: Due to missing callback interface and return value, fix memory leak.
}

void Preconnect::OnPreconnectComplete(int error_code) {
  delete this;
}

}  // namespace net
