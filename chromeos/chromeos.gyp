# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'chromeos',
      'type': '<(component)',
      'dependencies': [
        '../base/base.gyp:base',
        '../build/linux/system.gyp:dbus',
        '../dbus/dbus.gyp:dbus',
        '../net/net.gyp:net',
        'power_state_control_proto',
        'power_supply_properties_proto',
      ],
      'sources': [
        'chromeos_export.h',
        'dbus/blocking_method_caller.cc',
        'dbus/blocking_method_caller.h',
        'dbus/bluetooth_adapter_client.cc',
        'dbus/bluetooth_adapter_client.h',
        'dbus/bluetooth_agent_service_provider.cc',
        'dbus/bluetooth_agent_service_provider.h',
        'dbus/bluetooth_device_client.cc',
        'dbus/bluetooth_device_client.h',
        'dbus/bluetooth_input_client.cc',
        'dbus/bluetooth_input_client.h',
        'dbus/bluetooth_manager_client.cc',
        'dbus/bluetooth_manager_client.h',
        'dbus/bluetooth_node_client.cc',
        'dbus/bluetooth_node_client.h',
        'dbus/bluetooth_property.cc',
        'dbus/bluetooth_property.h',
        'dbus/cashew_client.cc',
        'dbus/cashew_client.h',
        'dbus/cros_disks_client.cc',
        'dbus/cros_disks_client.h',
        'dbus/cryptohome_client.cc',
        'dbus/cryptohome_client.h',
        'dbus/dbus_client_implementation_type.h',
        'dbus/dbus_method_call_status.h',
        'dbus/dbus_thread_manager.cc',
        'dbus/dbus_thread_manager.h',
        'dbus/debug_daemon_client.cc',
        'dbus/debug_daemon_client.h',
        'dbus/cryptohome_client.h',
        'dbus/flimflam_ipconfig_client.cc',
        'dbus/flimflam_ipconfig_client.h',
        'dbus/flimflam_client_helper.cc',
        'dbus/flimflam_client_helper.h',
        'dbus/flimflam_network_client.cc',
        'dbus/flimflam_network_client.h',
        'dbus/flimflam_profile_client.cc',
        'dbus/flimflam_profile_client.h',
        'dbus/image_burner_client.cc',
        'dbus/image_burner_client.h',
        'dbus/introspectable_client.cc',
        'dbus/introspectable_client.h',
        'dbus/power_manager_client.cc',
        'dbus/power_manager_client.h',
        'dbus/power_supply_status.cc',
        'dbus/power_supply_status.h',
        'dbus/session_manager_client.cc',
        'dbus/session_manager_client.h',
        'dbus/speech_synthesizer_client.cc',
        'dbus/speech_synthesizer_client.h',
        'dbus/update_engine_client.cc',
        'dbus/update_engine_client.h',
      ],
    },
    {
      # This target contains mocks that can be used to write unit tests.
      'target_name': 'chromeos_test_support',
      'type': 'static_library',
      'dependencies': [
        '../build/linux/system.gyp:dbus',
        '../testing/gmock.gyp:gmock',
        'chromeos',
      ],
      'sources': [
        'dbus/mock_bluetooth_adapter_client.cc',
        'dbus/mock_bluetooth_adapter_client.h',
        'dbus/mock_bluetooth_device_client.cc',
        'dbus/mock_bluetooth_device_client.h',
        'dbus/mock_bluetooth_input_client.cc',
        'dbus/mock_bluetooth_input_client.h',
        'dbus/mock_bluetooth_manager_client.cc',
        'dbus/mock_bluetooth_manager_client.h',
        'dbus/mock_bluetooth_node_client.cc',
        'dbus/mock_bluetooth_node_client.h',
        'dbus/mock_cros_disks_client.cc',
        'dbus/mock_cros_disks_client.h',
        'dbus/mock_cashew_client.cc',
        'dbus/mock_cashew_client.h',
        'dbus/mock_cryptohome_client.cc',
        'dbus/mock_cryptohome_client.h',
        'dbus/mock_dbus_thread_manager.cc',
        'dbus/mock_dbus_thread_manager.h',
        'dbus/mock_debug_daemon_client.cc',
        'dbus/mock_debug_daemon_client.h',
        'dbus/mock_flimflam_ipconfig_client.cc',
        'dbus/mock_flimflam_ipconfig_client.h',
        'dbus/mock_flimflam_network_client.cc',
        'dbus/mock_flimflam_network_client.h',
        'dbus/mock_flimflam_profile_client.cc',
        'dbus/mock_flimflam_profile_client.h',
        'dbus/mock_image_burner_client.cc',
        'dbus/mock_image_burner_client.h',
        'dbus/mock_introspectable_client.cc',
        'dbus/mock_introspectable_client.h',
        'dbus/mock_power_manager_client.cc',
        'dbus/mock_power_manager_client.h',
        'dbus/mock_session_manager_client.cc',
        'dbus/mock_session_manager_client.h',
        'dbus/mock_speech_synthesizer_client.cc',
        'dbus/mock_speech_synthesizer_client.h',
        'dbus/mock_update_engine_client.cc',
        'dbus/mock_update_engine_client.h',
      ],
      'include_dirs': [
        '..',
      ],
    },
    {
      'target_name': 'chromeos_unittests',
      'type': 'executable',
      'dependencies': [
        '../base/base.gyp:run_all_unittests',
        '../base/base.gyp:test_support_base',
        '../build/linux/system.gyp:dbus',
        '../dbus/dbus.gyp:dbus_test_support',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        'chromeos_test_support',
      ],
      'sources': [
        'dbus/blocking_method_caller_unittest.cc',
        'dbus/flimflam_client_unittest_base.cc',
        'dbus/flimflam_client_unittest_base.h',
        'dbus/flimflam_network_client_unittest.cc',
      ],
      'include_dirs': [
        '..',
      ],
    },
    {
      # Protobuf compiler / generator for the PowerSupplyProperties protocol
      # buffer.
      'target_name': 'power_state_control_proto',
      'type': 'static_library',
      'sources': [
        '../third_party/cros_system_api/dbus/power_state_control.proto',
      ],
      'variables': {
        'proto_in_dir': '../third_party/cros_system_api/dbus/',
        'proto_out_dir': 'chromeos/dbus',
      },
      'includes': ['../build/protoc.gypi'],
    },
    {
      # Protobuf compiler / generator for the PowerSupplyProperties protocol
      # buffer.
      'target_name': 'power_supply_properties_proto',
      'type': 'static_library',
      'sources': [
        '../third_party/cros_system_api/dbus/power_supply_properties.proto',
      ],
      'variables': {
        'proto_in_dir': '../third_party/cros_system_api/dbus/',
        'proto_out_dir': 'chromeos/dbus',
      },
      'includes': ['../build/protoc.gypi'],
    },
  ],
}
