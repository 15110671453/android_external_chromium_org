// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_SYSTEM_TRAY_DELEGATE_H_
#define ASH_SYSTEM_TRAY_SYSTEM_TRAY_DELEGATE_H_

#include <string>
#include <vector>

#include "ash/ash_export.h"
#include "ash/system/user/login_status.h"
#include "base/files/file_path.h"
#include "base/i18n/time_formatting.h"
#include "base/memory/scoped_ptr.h"
#include "base/string16.h"
#include "ui/gfx/image/image_skia.h"

namespace base {
class TimeDelta;
class TimeTicks;
}

namespace ash {

struct ASH_EXPORT NetworkIconInfo {
  NetworkIconInfo();
  ~NetworkIconInfo();

  bool highlight() const { return connected || connecting; }

  bool connecting;
  bool connected;
  bool tray_icon_visible;
  gfx::ImageSkia image;
  base::string16 name;
  base::string16 description;
  std::string service_path;
  bool is_cellular;
};

struct ASH_EXPORT BluetoothDeviceInfo {
  BluetoothDeviceInfo();
  ~BluetoothDeviceInfo();

  std::string address;
  base::string16 display_name;
  bool connected;
  bool connecting;
  bool paired;
};

typedef std::vector<BluetoothDeviceInfo> BluetoothDeviceList;

// Structure that packs progress information of each operation.
struct ASH_EXPORT DriveOperationStatus {
  enum OperationType {
    OPERATION_UPLOAD,
    OPERATION_DOWNLOAD
  };

  enum OperationState {
    OPERATION_NOT_STARTED,
    OPERATION_IN_PROGRESS,
    OPERATION_COMPLETED,
    OPERATION_FAILED,
  };

  DriveOperationStatus();
  ~DriveOperationStatus();

  // Unique ID for the operation.
  int32 id;

  // File path.
  base::FilePath file_path;
  // Current operation completion progress [0.0 - 1.0].
  double progress;
  OperationType type;
  OperationState state;
};

typedef std::vector<DriveOperationStatus> DriveOperationStatusList;


struct ASH_EXPORT IMEPropertyInfo {
  IMEPropertyInfo();
  ~IMEPropertyInfo();

  bool selected;
  std::string key;
  base::string16 name;
};

typedef std::vector<IMEPropertyInfo> IMEPropertyInfoList;

struct ASH_EXPORT IMEInfo {
  IMEInfo();
  ~IMEInfo();

  bool selected;
  bool third_party;
  std::string id;
  base::string16 name;
  base::string16 medium_name;
  base::string16 short_name;
};

typedef std::vector<IMEInfo> IMEInfoList;

class VolumeControlDelegate;

typedef std::vector<std::string> UserEmailList;

class SystemTrayDelegate {
 public:
  virtual ~SystemTrayDelegate() {}

  // Called after SystemTray has been instantiated.
  virtual void Initialize() = 0;

  // Called before SystemTray is destroyed.
  virtual void Shutdown() = 0;

  // Returns true if system tray should be visible on startup.
  virtual bool GetTrayVisibilityOnStartup() = 0;

  // Gets information about the active user.
  virtual const base::string16 GetUserDisplayName() const = 0;
  virtual const std::string GetUserEmail() const = 0;
  virtual const gfx::ImageSkia& GetUserImage() const = 0;
  virtual user::LoginStatus GetUserLoginStatus() const = 0;
  virtual bool IsOobeCompleted() const = 0;

  // Returns a list of all logged in users.
  virtual void GetLoggedInUsers(UserEmailList* users) = 0;

  // Switches to another active user (if that user has already signed in).
  virtual void SwitchActiveUser(const std::string& email) = 0;

  // Shows UI for changing user's profile picture.
  virtual void ChangeProfilePicture() = 0;

  // Returns the domain that manages the device, if it is enterprise-enrolled.
  virtual const std::string GetEnterpriseDomain() const = 0;

  // Returns notification for enterprise enrolled devices.
  virtual const base::string16 GetEnterpriseMessage() const = 0;

  // Returns the email of user that manages current locally managed user.
  virtual const std::string GetLocallyManagedUserManager() const = 0;

  // Returns notification for locally managed users.
  virtual const base::string16 GetLocallyManagedUserMessage() const = 0;

  // Returns whether a system upgrade is available.
  virtual bool SystemShouldUpgrade() const = 0;

  // Returns the desired hour clock type.
  virtual base::HourClockType GetHourClockType() const = 0;

  // Shows settings.
  virtual void ShowSettings() = 0;

  // Shows the settings related to date, timezone etc.
  virtual void ShowDateSettings() = 0;

  // Shows the settings related to network. If |service_path| is not empty,
  // show the settings for that network.
  virtual void ShowNetworkSettings(const std::string& service_path) = 0;

  // Shows the settings related to bluetooth.
  virtual void ShowBluetoothSettings() = 0;

  // Shows settings related to multiple displays.
  virtual void ShowDisplaySettings() = 0;

  // Shows settings related to Google Drive.
  virtual void ShowDriveSettings() = 0;

  // Shows settings related to input methods.
  virtual void ShowIMESettings() = 0;

  // Shows help.
  virtual void ShowHelp() = 0;

  // Show accessilibity help.
  virtual void ShowAccessibilityHelp() = 0;

  // Show the settings related to accessilibity.
  virtual void ShowAccessibilitySettings() = 0;

  // Shows more information about public account mode.
  virtual void ShowPublicAccountInfo() = 0;

  // Shows information about enterprise enrolled devices.
  virtual void ShowEnterpriseInfo() = 0;

  // Shows information about locally managed users.
  virtual void ShowLocallyManagedUserInfo() = 0;

  // Shows login UI to add other users to this session.
  virtual void ShowUserLogin() = 0;

  // Attempts to shut down the system.
  virtual void ShutDown() = 0;

  // Attempts to sign out the user.
  virtual void SignOut() = 0;

  // Attempts to lock the screen.
  virtual void RequestLockScreen() = 0;

  // Attempts to restart the system for update.
  virtual void RequestRestartForUpdate() = 0;

  // Returns a list of available bluetooth devices.
  virtual void GetAvailableBluetoothDevices(BluetoothDeviceList* devices) = 0;

  // Requests bluetooth start discovering devices.
  virtual void BluetoothStartDiscovering() = 0;

  // Requests bluetooth stop discovering devices.
  virtual void BluetoothStopDiscovering() = 0;

  // Connect to a specific bluetooth device.
  virtual void ConnectToBluetoothDevice(const std::string& address) = 0;

  // Returns true if bluetooth adapter is discovering bluetooth devices.
  virtual bool IsBluetoothDiscovering() = 0;

  // Returns the currently selected IME.
  virtual void GetCurrentIME(IMEInfo* info) = 0;

  // Returns a list of availble IMEs.
  virtual void GetAvailableIMEList(IMEInfoList* list) = 0;

  // Returns a list of properties for the currently selected IME.
  virtual void GetCurrentIMEProperties(IMEPropertyInfoList* list) = 0;

  // Switches to the selected input method.
  virtual void SwitchIME(const std::string& ime_id) = 0;

  // Activates an IME property.
  virtual void ActivateIMEProperty(const std::string& key) = 0;

  // Cancels ongoing drive operation.
  virtual void CancelDriveOperation(int32 operation_id) = 0;

  // Returns information about the ongoing drive operations.
  virtual void GetDriveOperationStatusList(
      DriveOperationStatusList* list) = 0;

  // Returns information about the most relevant network. Relevance is
  // determined by the implementor (e.g. a connecting network may be more
  // relevant over a connected network etc.)
  virtual void GetMostRelevantNetworkIcon(NetworkIconInfo* info,
                                          bool large) = 0;

  virtual void GetVirtualNetworkIcon(ash::NetworkIconInfo* info) = 0;

  // Returns information about the available networks.
  virtual void GetAvailableNetworks(std::vector<NetworkIconInfo>* list) = 0;

  // Returns the information about all virtual networks.
  virtual void GetVirtualNetworks(std::vector<NetworkIconInfo>* list) = 0;

  // Shows UI to configure or activate the network specified by |network_id|.
  virtual void ConfigureNetwork(const std::string& network_id) = 0;

  // Sends a connect request for the network specified by |network_id|.
  virtual void ConnectToNetwork(const std::string& network_id) = 0;

  // Gets the network IP address, and the mac addresses for the ethernet and
  // wifi devices. If any of this is unavailable, empty strings are returned.
  virtual void GetNetworkAddresses(std::string* ip_address,
                                   std::string* ethernet_mac_address,
                                   std::string* wifi_mac_address) = 0;

  // Requests network scan when list of networks is opened.
  virtual void RequestNetworkScan() = 0;

  // Shous UI to add a new bluetooth device.
  virtual void AddBluetoothDevice() = 0;

  // Toggles airplane mode.
  virtual void ToggleAirplaneMode() = 0;

  // Toggles wifi network.
  virtual void ToggleWifi() = 0;

  // Toggles mobile network.
  virtual void ToggleMobile() = 0;

  // Toggles bluetooth.
  virtual void ToggleBluetooth() = 0;

  // Shows UI to unlock a mobile sim.
  virtual void ShowMobileSimDialog() = 0;

  // Shows UI to connect to an unlisted wifi network.
  virtual void ShowOtherWifi() = 0;

  // Shows UI to configure vpn.
  virtual void ShowOtherVPN() = 0;

  // Shows UI to search for cellular networks.
  virtual void ShowOtherCellular() = 0;

  // Returns whether the system is connected to any network.
  virtual bool IsNetworkConnected() = 0;

  // Returns whether wifi is available.
  virtual bool GetWifiAvailable() = 0;

  // Returns whether mobile networking (cellular or wimax) is available.
  virtual bool GetMobileAvailable() = 0;

  // Returns whether bluetooth capability is available.
  virtual bool GetBluetoothAvailable() = 0;

  // Returns whether wifi is enabled.
  virtual bool GetWifiEnabled() = 0;

  // Returns whether mobile (cellular or wimax) networking is enabled.
  virtual bool GetMobileEnabled() = 0;

  // Returns whether bluetooth is enabled.
  virtual bool GetBluetoothEnabled() = 0;

  // Returns whether mobile scanning is supported.
  virtual bool GetMobileScanSupported() = 0;

  // Retrieves information about the carrier and locale specific |setup_url|.
  // If none of the carrier info/setup URL cannot be retrieved, returns false.
  // Note: |setup_url| is returned when carrier is not defined (no SIM card).
  virtual bool GetCellularCarrierInfo(std::string* carrier_id,
                                      std::string* topup_url,
                                      std::string* setup_url) = 0;

  // Returns whether the network manager is scanning for wifi networks.
  virtual bool GetWifiScanning() = 0;

  // Returns whether the network manager is initializing the cellular modem.
  virtual bool GetCellularInitializing() = 0;

  // Opens the cellular network specific URL.
  virtual void ShowCellularURL(const std::string& url) = 0;

  // Shows UI for changing proxy settings.
  virtual void ChangeProxySettings() = 0;

  // Returns VolumeControlDelegate.
  virtual VolumeControlDelegate* GetVolumeControlDelegate() const = 0;

  // Sets VolumeControlDelegate.
  virtual void SetVolumeControlDelegate(
      scoped_ptr<VolumeControlDelegate> delegate) = 0;

  // Retrieves the session start time. Returns |false| if the time is not set.
  virtual bool GetSessionStartTime(base::TimeTicks* session_start_time) = 0;

  // Retrieves the session length limit. Returns |false| if no limit is set.
  virtual bool GetSessionLengthLimit(base::TimeDelta* session_length_limit) = 0;

  // Get the system tray menu size in pixels (dependent on the language).
  virtual int GetSystemTrayMenuWidth() = 0;

  // Returns the duration formatted as a localized string.
  // TODO(stevenjb): Move TimeFormat from src/chrome to src/ui so that it can be
  // accessed without going through the delegate. crbug.com/222697
  virtual base::string16 FormatTimeDuration(
      const base::TimeDelta& delta) const = 0;

  // Speaks the given text if spoken feedback is enabled.
  virtual void MaybeSpeak(const std::string& utterance) const = 0;

  // Creates a dummy delegate for testing.
  static SystemTrayDelegate* CreateDummyDelegate();
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_SYSTEM_TRAY_DELEGATE_H_
