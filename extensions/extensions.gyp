# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'extensions_common',
      'type': 'static_library',
      'dependencies': [
        'common/api/api.gyp:extensions_api',
        # TODO(benwells): figure out what to do with the api target and
        # api resources compiled into the chrome resource bundle.
        # http://crbug.com/162530
        '../chrome/chrome_resources.gyp:chrome_resources',
        # TODO(jamescook|derat): Pull strings into extensions module.
        '../chrome/chrome_resources.gyp:chrome_strings',
        # TODO(tfarina): This dep here is for extensions/common/constants.*
        # We should find a way to compile this module within extensions_common.
        '../chrome/common_constants.gyp:common_constants',
        '../components/components.gyp:url_matcher',
        '../content/content.gyp:content_common',
        '../crypto/crypto.gyp:crypto',
        '../ipc/ipc.gyp:ipc',
        '../net/net.gyp:net',
        '../third_party/re2/re2.gyp:re2',
        '../ui/base/ui_base.gyp:ui_base',
        '../ui/gfx/gfx.gyp:gfx_geometry',
        '../url/url.gyp:url_lib',
      ],
      'include_dirs': [
        '..',
        '<(INTERMEDIATE_DIR)',
      ],
      'sources': [
        'common/api/messaging/message.h',
        'common/api/sockets/sockets_manifest_data.cc',
        'common/api/sockets/sockets_manifest_data.h',
        'common/api/sockets/sockets_manifest_handler.cc',
        'common/api/sockets/sockets_manifest_handler.h',
        'common/api/sockets/sockets_manifest_permission.cc',
        'common/api/sockets/sockets_manifest_permission.h',
        'common/common_manifest_handlers.cc',
        'common/common_manifest_handlers.h',
        'common/crx_file.cc',
        'common/crx_file.h',
        'common/csp_validator.cc',
        'common/csp_validator.h',
        'common/draggable_region.cc',
        'common/draggable_region.h',
        'common/error_utils.cc',
        'common/error_utils.h',
        'common/event_filter.cc',
        'common/event_filter.h',
        'common/event_filtering_info.cc',
        'common/event_filtering_info.h',
        'common/event_matcher.cc',
        'common/event_matcher.h',
        'common/extension.cc',
        'common/extension.h',
        'common/extension_api.cc',
        'common/extension_api.h',
        'common/extension_api_stub.cc',
        'common/extension_icon_set.cc',
        'common/extension_icon_set.h',
        'common/extension_l10n_util.cc',
        'common/extension_l10n_util.h',
        'common/extension_message_generator.cc',
        'common/extension_message_generator.h',
        'common/extension_messages.cc',
        'common/extension_messages.h',
        'common/extension_paths.cc',
        'common/extension_paths.h',
        'common/extension_resource.cc',
        'common/extension_resource.h',
        'common/extension_set.cc',
        'common/extension_set.h',
        'common/extension_urls.cc',
        'common/extension_urls.h',
        'common/extensions_client.cc',
        'common/extensions_client.h',
        'common/feature_switch.cc',
        'common/feature_switch.h',
        'common/features/api_feature.cc',
        'common/features/api_feature.h',
        'common/features/base_feature_provider.cc',
        'common/features/base_feature_provider.h',
        'common/features/complex_feature.cc',
        'common/features/complex_feature.h',
        'common/features/feature.cc',
        'common/features/feature.h',
        'common/features/feature_provider.cc',
        'common/features/feature_provider.h',
        'common/features/manifest_feature.cc',
        'common/features/manifest_feature.h',
        'common/features/permission_feature.cc',
        'common/features/permission_feature.h',
        'common/features/simple_feature.cc',
        'common/features/simple_feature.h',
        'common/features/simple_feature_filter.cc',
        'common/features/simple_feature_filter.h',
        'common/file_util.cc',
        'common/file_util.h',
        'common/id_util.cc',
        'common/id_util.h',
        'common/install_warning.cc',
        'common/install_warning.h',
        'common/manifest.cc',
        'common/manifest.h',
        'common/manifest_constants.cc',
        'common/manifest_constants.h',
        'common/manifest_handler.cc',
        'common/manifest_handler.h',
        'common/manifest_handler_helpers.cc',
        'common/manifest_handler_helpers.h',
        'common/manifest_handlers/background_info.cc',
        'common/manifest_handlers/background_info.h',
        'common/manifest_handlers/csp_info.cc',
        'common/manifest_handlers/csp_info.h',
        'common/manifest_handlers/incognito_info.cc',
        'common/manifest_handlers/incognito_info.h',
        'common/manifest_handlers/kiosk_mode_info.cc',
        'common/manifest_handlers/kiosk_mode_info.h',
        'common/manifest_handlers/offline_enabled_info.cc',
        'common/manifest_handlers/offline_enabled_info.h',
        'common/manifest_handlers/requirements_info.cc',
        'common/manifest_handlers/requirements_info.h',
        'common/manifest_handlers/sandboxed_page_info.cc',
        'common/manifest_handlers/sandboxed_page_info.h',
        'common/manifest_handlers/shared_module_info.cc',
        'common/manifest_handlers/shared_module_info.h',
        'common/manifest_handlers/web_accessible_resources_info.cc',
        'common/manifest_handlers/web_accessible_resources_info.h',
        'common/manifest_handlers/webview_info.cc',
        'common/manifest_handlers/webview_info.h',
        'common/message_bundle.cc',
        'common/message_bundle.h',
        'common/one_shot_event.cc',
        'common/one_shot_event.h',
        'common/permissions/api_permission.cc',
        'common/permissions/api_permission.h',
        'common/permissions/api_permission_set.cc',
        'common/permissions/api_permission_set.h',
        'common/permissions/base_set_operators.h',
        'common/permissions/manifest_permission.cc',
        'common/permissions/manifest_permission.h',
        'common/permissions/manifest_permission_set.cc',
        'common/permissions/manifest_permission_set.h',
        'common/permissions/media_galleries_permission.cc',
        'common/permissions/media_galleries_permission.h',
        'common/permissions/media_galleries_permission_data.cc',
        'common/permissions/media_galleries_permission_data.h',
        'common/permissions/permission_message.cc',
        'common/permissions/permission_message.h',
        'common/permissions/permission_message_provider.cc',
        'common/permissions/permission_message_provider.h',
        'common/permissions/permission_message_util.cc',
        'common/permissions/permission_message_util.h',
        'common/permissions/permission_set.cc',
        'common/permissions/permission_set.h',
        'common/permissions/permissions_data.cc',
        'common/permissions/permissions_data.h',
        'common/permissions/permissions_info.cc',
        'common/permissions/permissions_info.h',
        'common/permissions/permissions_provider.h',
        'common/permissions/set_disjunction_permission.h',
        'common/permissions/settings_override_permission.cc',
        'common/permissions/settings_override_permission.h',
        'common/permissions/socket_permission.cc',
        'common/permissions/socket_permission.h',
        'common/permissions/socket_permission_data.cc',
        'common/permissions/socket_permission_data.h',
        'common/permissions/socket_permission_entry.cc',
        'common/permissions/socket_permission_entry.h',
        'common/permissions/usb_device_permission.cc',
        'common/permissions/usb_device_permission.h',
        'common/permissions/usb_device_permission_data.cc',
        'common/permissions/usb_device_permission_data.h',
        'common/stack_frame.cc',
        'common/stack_frame.h',
        'common/switches.cc',
        'common/switches.h',
        'common/url_pattern.cc',
        'common/url_pattern.h',
        'common/url_pattern_set.cc',
        'common/url_pattern_set.h',
        'common/user_script.cc',
        'common/user_script.h',
        'common/view_type.cc',
        'common/view_type.h',
      ],
      # Disable c4267 warnings until we fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
      'conditions': [
        ['enable_extensions==1', {
          'sources!': [
            'common/extension_api_stub.cc',
          ],
        }, {  # enable_extensions == 0
          'sources!': [
            'common/extension_api.cc',
          ],
        }],
      ],
    },
    {
      'target_name': 'extensions_browser',
      'type': 'static_library',
      'dependencies': [
        'extensions_common',
        'common/api/api.gyp:extensions_api',
        # TODO(jamescook|derat): Pull strings into extensions module.
        '../chrome/chrome_resources.gyp:chrome_strings',
        '../components/components.gyp:keyed_service_content',
        '../content/content.gyp:content_browser',
        '../skia/skia.gyp:skia',
        '../third_party/leveldatabase/leveldatabase.gyp:leveldatabase',
      ],
      'include_dirs': [
        '..',
        '<(INTERMEDIATE_DIR)',
        # Needed to access generated API headers.
        '<(SHARED_INTERMEDIATE_DIR)',
        # Needed for grit.
        '<(SHARED_INTERMEDIATE_DIR)/chrome',
      ],
      'sources': [
        'browser/admin_policy.cc',
        'browser/admin_policy.h',
        # NOTE: When moving an API out of Chrome be sure to verify that the
        # Android build still compiles. See conditions below.
        'browser/api/api_resource.cc',
        'browser/api/api_resource.h',
        'browser/api/api_resource_manager.h',
        'browser/api/async_api_function.cc',
        'browser/api/async_api_function.h',
        'browser/api/dns/dns_api.cc',
        'browser/api/dns/dns_api.h',
        'browser/api/dns/host_resolver_wrapper.cc',
        'browser/api/dns/host_resolver_wrapper.h',
        'browser/api/extensions_api_client.cc',
        'browser/api/extensions_api_client.h',
        'browser/api/socket/socket.cc',
        'browser/api/socket/socket.h',
        'browser/api/socket/socket_api.cc',
        'browser/api/socket/socket_api.h',
        'browser/api/socket/tcp_socket.cc',
        'browser/api/socket/tcp_socket.h',
        'browser/api/socket/udp_socket.cc',
        'browser/api/socket/udp_socket.h',
        'browser/api/sockets_tcp/sockets_tcp_api.cc',
        'browser/api/sockets_tcp/sockets_tcp_api.h',
        'browser/api/sockets_tcp/tcp_socket_event_dispatcher.cc',
        'browser/api/sockets_tcp/tcp_socket_event_dispatcher.h',
        'browser/api/sockets_tcp_server/sockets_tcp_server_api.cc',
        'browser/api/sockets_tcp_server/sockets_tcp_server_api.h',
        'browser/api/sockets_tcp_server/tcp_server_socket_event_dispatcher.cc',
        'browser/api/sockets_tcp_server/tcp_server_socket_event_dispatcher.h',
        'browser/api/sockets_udp/sockets_udp_api.cc',
        'browser/api/sockets_udp/sockets_udp_api.h',
        'browser/api/sockets_udp/udp_socket_event_dispatcher.cc',
        'browser/api/sockets_udp/udp_socket_event_dispatcher.h',
        'browser/api/storage/leveldb_settings_storage_factory.cc',
        'browser/api/storage/leveldb_settings_storage_factory.h',
        'browser/api/storage/local_value_store_cache.cc',
        'browser/api/storage/local_value_store_cache.h',
        'browser/api/storage/settings_namespace.cc',
        'browser/api/storage/settings_namespace.h',
        'browser/api/storage/settings_observer.h',
        'browser/api/storage/settings_storage_factory.h',
        'browser/api/storage/settings_storage_quota_enforcer.cc',
        'browser/api/storage/settings_storage_quota_enforcer.h',
        'browser/api/storage/storage_api.cc',
        'browser/api/storage/storage_api.h',
        'browser/api/storage/storage_frontend.cc',
        'browser/api/storage/storage_frontend.h',
        'browser/api/storage/value_store_cache.cc',
        'browser/api/storage/value_store_cache.h',
        'browser/api/storage/weak_unlimited_settings_storage.cc',
        'browser/api/storage/weak_unlimited_settings_storage.h',
        'browser/api/test/test_api.cc',
        'browser/api/test/test_api.h',
        'browser/api_activity_monitor.h',
        'browser/app_sorting.h',
        'browser/blacklist_state.h',
        'browser/browser_context_keyed_api_factory.h',
        'browser/browser_context_keyed_service_factories.cc',
        'browser/browser_context_keyed_service_factories.h',
        'browser/error_map.cc',
        'browser/error_map.h',
        'browser/event_listener_map.cc',
        'browser/event_listener_map.h',
        'browser/event_router.cc',
        'browser/event_router.h',
        'browser/extension_host.cc',
        'browser/extension_host.h',
        'browser/extension_host_delegate.h',
        'browser/extension_error.cc',
        'browser/extension_error.h',
        'browser/extension_function.cc',
        'browser/extension_function.h',
        'browser/extension_function_dispatcher.cc',
        'browser/extension_function_dispatcher.h',
        'browser/extension_function_registry.cc',
        'browser/extension_function_registry.h',
        'browser/extension_function_util.cc',
        'browser/extension_function_util.h',
        'browser/extension_message_filter.cc',
        'browser/extension_message_filter.h',
        'browser/extension_pref_store.cc',
        'browser/extension_pref_store.h',
        'browser/extension_pref_value_map.cc',
        'browser/extension_pref_value_map_factory.cc',
        'browser/extension_pref_value_map_factory.h',
        'browser/extension_pref_value_map.h',
        'browser/extension_prefs.cc',
        'browser/extension_prefs.h',
        'browser/extension_prefs_factory.cc',
        'browser/extension_prefs_factory.h',
        'browser/extension_prefs_observer.h',
        'browser/extension_prefs_scope.h',
        'browser/extension_registry.cc',
        'browser/extension_registry.h',
        'browser/extension_registry_factory.cc',
        'browser/extension_registry_factory.h',
        'browser/extension_registry_observer.h',
        'browser/extension_scoped_prefs.h',
        'browser/extension_system.cc',
        'browser/extension_system.h',
        'browser/extension_system_provider.cc',
        'browser/extension_system_provider.h',
        'browser/extension_web_contents_observer.cc',
        'browser/extension_web_contents_observer.h',
        'browser/extensions_browser_client.cc',
        'browser/extensions_browser_client.h',
        'browser/external_provider_interface.h',
        'browser/image_util.cc',
        'browser/image_util.h',
        'browser/info_map.cc',
        'browser/info_map.h',
        'browser/file_highlighter.cc',
        'browser/file_highlighter.h',
        'browser/file_reader.cc',
        'browser/file_reader.h',
        'browser/lazy_background_task_queue.cc',
        'browser/lazy_background_task_queue.h',
        'browser/management_policy.cc',
        'browser/management_policy.h',
        'browser/pref_names.cc',
        'browser/pref_names.h',
        'browser/process_manager.cc',
        'browser/process_manager.h',
        'browser/process_manager_observer.h',
        'browser/process_map.cc',
        'browser/process_map.h',
        'browser/process_map_factory.cc',
        'browser/process_map_factory.h',
        'browser/quota_service.cc',
        'browser/quota_service.h',
        'browser/renderer_startup_helper.cc',
        'browser/renderer_startup_helper.h',
        'browser/runtime_data.cc',
        'browser/runtime_data.h',
        'browser/update_observer.h',
        'browser/value_store/leveldb_value_store.cc',
        'browser/value_store/leveldb_value_store.h',
        'browser/value_store/testing_value_store.cc',
        'browser/value_store/testing_value_store.h',
        'browser/value_store/value_store.cc',
        'browser/value_store/value_store.h',
        'browser/value_store/value_store_change.cc',
        'browser/value_store/value_store_change.h',
        'browser/value_store/value_store_frontend.cc',
        'browser/value_store/value_store_frontend.h',
        'browser/value_store/value_store_util.cc',
        'browser/value_store/value_store_util.h',
        'browser/view_type_utils.cc',
        'browser/view_type_utils.h',
      ],
      'conditions': [
        ['enable_extensions==0', {
          # Exclude all API implementations and the ExtensionsApiClient
          # interface. Moving an API from src/chrome to src/extensions implies
          # it can be cleanly disabled with enable_extensions==0.
          # TODO: Eventually the entire extensions module should not be built
          # when enable_extensions==0.
          'sources/': [
            ['exclude', '^browser/api/'],
          ],
          'sources!': [
            'browser/browser_context_keyed_service_factories.cc',
            'browser/browser_context_keyed_service_factories.h',
          ],
        }],
      ],
      # Disable c4267 warnings until we fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
    {
      'target_name': 'extensions_renderer',
      'type': 'static_library',
      'include_dirs': [
        '..',
      ],
      'sources': [
        'renderer/native_handler.cc',
        'renderer/native_handler.h',
        'renderer/scoped_persistent.h',
      ],
      'dependencies': [
        '../third_party/WebKit/public/blink.gyp:blink',
      ],
      # Disable c4267 warnings until we fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
    {
      'target_name': 'extensions_test_support',
      'type': 'static_library',
      'dependencies': [
        'extensions_browser',
        'extensions_common',
        '../base/base.gyp:base',
        '../testing/gtest.gyp:gtest',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'browser/test_extensions_browser_client.cc',
        'browser/test_extensions_browser_client.h',
        'browser/test_management_policy.cc',
        'browser/test_management_policy.h',
        'common/extension_builder.cc',
        'common/extension_builder.h',
        'common/test_util.cc',
        'common/test_util.h',
        'common/value_builder.cc',
        'common/value_builder.h',
      ],
      # Disable c4267 warnings until we fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
    {
      # TODO(tfarina): Our plan is to build and run this target on Chromium bots
      # (TS, CQ, Waterfall). First we will get this target passing all tests,
      # after that we will start the work on buildbot to get this running there.
      # When we consider this stable in the bots, we can go to unit_tests target
      # and remove the duplicated entries from there, otherwise if we just
      # remove them right now we would be losing coverage.
      # http://crbug.com/348066
      'target_name': 'extensions_unittests',
      'type': 'executable',
      'dependencies': [
        '../base/base.gyp:base',
        '../base/base.gyp:run_all_unittests',
        '../base/base.gyp:test_support_base',
        '../testing/gtest.gyp:gtest',
        'extensions_common',
        'extensions_test_support',
      ],
      'sources': [
        'common/api/sockets/sockets_manifest_permission_unittest.cc',
        'common/csp_validator_unittest.cc',
        'common/event_filter_unittest.cc',
        'common/file_util_unittest.cc',
        'common/id_util_unittest.cc',
        'common/manifest_handler_unittest.cc',
        'common/one_shot_event_unittest.cc',
        'common/permissions/api_permission_set_unittest.cc',
        'common/permissions/manifest_permission_set_unittest.cc',
        'common/url_pattern_set_unittest.cc',
        'common/url_pattern_unittest.cc',
        'common/user_script_unittest.cc',
      ],
    },
  ]
}
