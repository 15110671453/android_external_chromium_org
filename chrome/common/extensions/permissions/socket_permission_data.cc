// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/permissions/socket_permission_data.h"

#include <cstdlib>
#include <sstream>
#include <vector>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "chrome/common/extensions/permissions/socket_permission.h"
#include "extensions/common/permissions/api_permission.h"
#include "url/url_canon.h"

namespace {

using content::SocketPermissionRequest;
using extensions::SocketPermissionData;

const char kColon = ':';
const char kInvalid[] = "invalid";
const char kTCPConnect[] = "tcp-connect";
const char kTCPListen[] = "tcp-listen";
const char kUDPBind[] = "udp-bind";
const char kUDPSendTo[] = "udp-send-to";
const char kUDPMulticastMembership[] = "udp-multicast-membership";
const char kResolveHost[] = "resolve-host";
const char kResolveProxy[] = "resolve-proxy";
const char kNetworkState[] = "network-state";

SocketPermissionRequest::OperationType StringToType(const std::string& s) {
  if (s == kTCPConnect)
    return SocketPermissionRequest::TCP_CONNECT;
  if (s == kTCPListen)
    return SocketPermissionRequest::TCP_LISTEN;
  if (s == kUDPBind)
    return SocketPermissionRequest::UDP_BIND;
  if (s == kUDPSendTo)
    return SocketPermissionRequest::UDP_SEND_TO;
  if (s == kUDPMulticastMembership)
    return SocketPermissionRequest::UDP_MULTICAST_MEMBERSHIP;
  if (s == kResolveHost)
    return SocketPermissionRequest::RESOLVE_HOST;
  if (s == kResolveProxy)
    return SocketPermissionRequest::RESOLVE_PROXY;
  if (s == kNetworkState)
    return SocketPermissionRequest::NETWORK_STATE;
  return SocketPermissionRequest::NONE;
}

const char* TypeToString(SocketPermissionRequest::OperationType type) {
  switch (type) {
    case SocketPermissionRequest::TCP_CONNECT:
      return kTCPConnect;
    case SocketPermissionRequest::TCP_LISTEN:
      return kTCPListen;
    case SocketPermissionRequest::UDP_BIND:
      return kUDPBind;
    case SocketPermissionRequest::UDP_SEND_TO:
      return kUDPSendTo;
    case SocketPermissionRequest::UDP_MULTICAST_MEMBERSHIP:
      return kUDPMulticastMembership;
    case SocketPermissionRequest::RESOLVE_HOST:
      return kResolveHost;
    case SocketPermissionRequest::RESOLVE_PROXY:
      return kResolveProxy;
    case SocketPermissionRequest::NETWORK_STATE:
      return kNetworkState;
    default:
      return kInvalid;
  }
}

}  // namespace

namespace extensions {

SocketPermissionData::SocketPermissionData() { }

SocketPermissionData::~SocketPermissionData() { }

bool SocketPermissionData::operator<(const SocketPermissionData& rhs) const {
  return entry_ < rhs.entry_;
}

bool SocketPermissionData::operator==(const SocketPermissionData& rhs) const {
  return entry_ == rhs.entry_;
}

bool SocketPermissionData::Check(
    const APIPermission::CheckParam* param) const {
  if (!param)
    return false;
  const SocketPermission::CheckParam& specific_param =
      *static_cast<const SocketPermission::CheckParam*>(param);
  const SocketPermissionRequest &request = specific_param.request;

  return entry_.Check(request);
}

scoped_ptr<base::Value> SocketPermissionData::ToValue() const {
  return scoped_ptr<base::Value>(new base::StringValue(GetAsString()));
}

bool SocketPermissionData::FromValue(const base::Value* value) {
  std::string spec;
  if (!value->GetAsString(&spec))
    return false;

  return Parse(spec);
}

SocketPermissionEntry& SocketPermissionData::entry() {
   // Clear the spec because the caller could mutate |this|.
   spec_.clear();
  return entry_;
}

// TODO(ikarienator): Rewrite this method to support IPv6.
bool SocketPermissionData::Parse(const std::string& permission) {
  Reset();

  std::vector<std::string> tokens;
  base::SplitStringDontTrim(permission, kColon, &tokens);
  if (tokens.empty())
    return false;

  SocketPermissionRequest::OperationType type = StringToType(tokens[0]);
  if (type == SocketPermissionRequest::NONE)
    return false;

  tokens.erase(tokens.begin());
  return SocketPermissionEntry::ParseHostPattern(type, tokens, &entry_);
}

const std::string& SocketPermissionData::GetAsString() const {
  if (!spec_.empty())
    return spec_;

  spec_.reserve(64);
  spec_.append(TypeToString(entry_.pattern().type));
  std::string pattern = entry_.GetHostPatternAsString();
  if (!pattern.empty()) {
    spec_.append(1, kColon).append(pattern);
  }
  return spec_;
}

void SocketPermissionData::Reset() {
  entry_ = SocketPermissionEntry();
  spec_.clear();
}

}  // namespace extensions
