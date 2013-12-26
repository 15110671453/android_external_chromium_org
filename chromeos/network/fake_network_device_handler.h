// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_FAKE_NETWORK_DEVICE_HANDLER_H_
#define CHROMEOS_NETWORK_FAKE_NETWORK_DEVICE_HANDLER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/network/network_device_handler.h"

namespace chromeos {

// This is a fake implementation which does nothing. Use this as a base class
// for concrete fake handlers.
class CHROMEOS_EXPORT FakeNetworkDeviceHandler : public NetworkDeviceHandler {
 public:
  FakeNetworkDeviceHandler();
  virtual ~FakeNetworkDeviceHandler();

  // NetworkDeviceHandler overrides
  virtual void GetDeviceProperties(
      const std::string& device_path,
      const network_handler::DictionaryResultCallback& callback,
      const network_handler::ErrorCallback& error_callback) const OVERRIDE;

  virtual void SetDeviceProperty(
      const std::string& device_path,
      const std::string& property_name,
      const base::Value& value,
      const base::Closure& callback,
      const network_handler::ErrorCallback& error_callback) OVERRIDE;

  virtual void RequestRefreshIPConfigs(
      const std::string& device_path,
      const base::Closure& callback,
      const network_handler::ErrorCallback& error_callback) OVERRIDE;

  virtual void ProposeScan(const std::string& device_path,
                           const base::Closure& callback,
                           const network_handler::ErrorCallback& error_callback)
      OVERRIDE;

  virtual void RegisterCellularNetwork(
      const std::string& device_path,
      const std::string& network_id,
      const base::Closure& callback,
      const network_handler::ErrorCallback& error_callback) OVERRIDE;

  virtual void SetCarrier(const std::string& device_path,
                          const std::string& carrier,
                          const base::Closure& callback,
                          const network_handler::ErrorCallback& error_callback)
      OVERRIDE;

  virtual void RequirePin(const std::string& device_path,
                          bool require_pin,
                          const std::string& pin,
                          const base::Closure& callback,
                          const network_handler::ErrorCallback& error_callback)
      OVERRIDE;

  virtual void EnterPin(const std::string& device_path,
                        const std::string& pin,
                        const base::Closure& callback,
                        const network_handler::ErrorCallback& error_callback)
      OVERRIDE;

  virtual void UnblockPin(const std::string& device_path,
                          const std::string& puk,
                          const std::string& new_pin,
                          const base::Closure& callback,
                          const network_handler::ErrorCallback& error_callback)
      OVERRIDE;

  virtual void ChangePin(const std::string& device_path,
                         const std::string& old_pin,
                         const std::string& new_pin,
                         const base::Closure& callback,
                         const network_handler::ErrorCallback& error_callback)
      OVERRIDE;

  virtual void SetCellularAllowRoaming(bool allow_roaming) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeNetworkDeviceHandler);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_FAKE_NETWORK_DEVICE_HANDLER_H_
