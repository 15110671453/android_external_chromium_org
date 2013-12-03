// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BLUETOOTH_INPUT_CLIENT_H_
#define CHROMEOS_DBUS_BLUETOOTH_INPUT_CLIENT_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/observer_list.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "dbus/object_path.h"
#include "dbus/property.h"

namespace chromeos {

// BluetoothInputClient is used to communicate with objects representing
// Bluetooth Input (HID) devices.
class CHROMEOS_EXPORT BluetoothInputClient : public DBusClient {
 public:
  // Structure of properties associated with bluetooth input devices.
  struct Properties : public dbus::PropertySet {
    // The Bluetooth input device reconnect mode. Read-only.
    dbus::Property<std::string> reconnect_mode;

    Properties(dbus::ObjectProxy* object_proxy,
               const std::string& interface_name,
               const PropertyChangedCallback& callback);
    virtual ~Properties();
  };

  // Interface for observing changes from a remote bluetooth input device.
  class Observer {
   public:
    virtual ~Observer() {}

    // Called when the remote device with object path |object_path| implementing
    // the Input interface is added to the set of known devices or an already
    // known device implements the Input interface.
    virtual void InputAdded(const dbus::ObjectPath& object_path) {}

    // Called when the remote device with object path |object_path| is removed
    // from the set of known devices or does not implement the Input interface
    // anymore.
    virtual void InputRemoved(const dbus::ObjectPath& object_path) {}

    // Called when the device with object path |object_path| has a
    // change in value of the property named |property_name| of its Input
    // interface.
    virtual void InputPropertyChanged(const dbus::ObjectPath& object_path,
                                      const std::string& property_name) {}
  };

  virtual ~BluetoothInputClient();

  // Adds and removes observers for events on all remote bluetooth input
  // devices. Check the |object_path| parameter of observer methods to
  // determine which device is issuing the event.
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Obtain the properties for the device with object path |object_path|,
  // any values should be copied if needed.
  virtual Properties* GetProperties(const dbus::ObjectPath& object_path) = 0;

  // Creates the instance.
  static BluetoothInputClient* Create();

 protected:
  BluetoothInputClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothInputClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_BLUETOOTH_INPUT_CLIENT_H_
