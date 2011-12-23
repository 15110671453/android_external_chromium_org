# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },

  'targets': [
    {
      'target_name': 'aura_shell',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../../build/temp_gyp/googleurl.gyp:googleurl',
        '../../net/net.gyp:net',
        '../../skia/skia.gyp:skia',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../aura/aura.gyp:aura',
        '../base/strings/ui_strings.gyp:ui_strings',
        '../gfx/compositor/compositor.gyp:compositor',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
        '../ui.gyp:ui_resources_standard',
        '../views/views.gyp:views',
      ],
      'defines': [
        'AURA_SHELL_IMPLEMENTATION',
      ],
      'sources': [
        # All .cc, .h under views, except unittests
        'aura_shell_switches.cc',
        'aura_shell_switches.h',
        'shell.cc',
        'shell.h',
        'shell_delegate.h',
        'shell_factory.h',
        'shell_window_ids.h',
        '../../ash/accelerators/accelerator_controller.cc',
        '../../ash/accelerators/accelerator_controller.h',
        '../../ash/accelerators/accelerator_filter.cc',
        '../../ash/accelerators/accelerator_filter.h',
        '../../ash/app_list/app_list.cc',
        '../../ash/app_list/app_list.h',
        '../../ash/app_list/app_list_groups_view.cc',
        '../../ash/app_list/app_list_groups_view.h',
        '../../ash/app_list/app_list_item_group_model.cc',
        '../../ash/app_list/app_list_item_group_model.h',
        '../../ash/app_list/app_list_item_group_view.cc',
        '../../ash/app_list/app_list_item_group_view.h',
        '../../ash/app_list/app_list_item_model.cc',
        '../../ash/app_list/app_list_item_model.h',
        '../../ash/app_list/app_list_item_model_observer.h',
        '../../ash/app_list/app_list_item_view.cc',
        '../../ash/app_list/app_list_item_view.h',
        '../../ash/app_list/app_list_item_view_listener.h',
        '../../ash/app_list/app_list_model.cc',
        '../../ash/app_list/app_list_model.h',
        '../../ash/app_list/app_list_view.cc',
        '../../ash/app_list/app_list_view.h',
        '../../ash/app_list/app_list_view_delegate.h',
        '../../ash/app_list/drop_shadow_label.cc',
        '../../ash/app_list/drop_shadow_label.h',
        '../../ash/desktop_background/desktop_background_view.cc',
        '../../ash/desktop_background/desktop_background_view.h',
        '../../ash/drag_drop/drag_drop_controller.cc',
        '../../ash/drag_drop/drag_drop_controller.h',
        '../../ash/drag_drop/drag_image_view.cc',
        '../../ash/drag_drop/drag_image_view.h',
        '../../ash/launcher/app_launcher_button.cc',
        '../../ash/launcher/app_launcher_button.h',
        '../../ash/launcher/launcher.cc',
        '../../ash/launcher/launcher.h',
        '../../ash/launcher/launcher_model.cc',
        '../../ash/launcher/launcher_model.h',
        '../../ash/launcher/launcher_model_observer.h',
        '../../ash/launcher/launcher_types.h',
        '../../ash/launcher/launcher_view.cc',
        '../../ash/launcher/launcher_view.h',
        '../../ash/launcher/tabbed_launcher_button.cc',
        '../../ash/launcher/tabbed_launcher_button.h',
        '../../ash/launcher/view_model.cc',
        '../../ash/launcher/view_model.h',
        '../../ash/launcher/view_model_utils.cc',
        '../../ash/launcher/view_model_utils.h',
        '../../ash/status_area/status_area_view.cc',
        '../../ash/status_area/status_area_view.h',
        '../../ash/tooltips/tooltip_controller.cc',
        '../../ash/tooltips/tooltip_controller.h',
        '../../ash/wm/activation_controller.cc',
        '../../ash/wm/activation_controller.h',
        '../../ash/wm/always_on_top_controller.cc',
        '../../ash/wm/always_on_top_controller.h',
        '../../ash/wm/compact_layout_manager.cc',
        '../../ash/wm/compact_layout_manager.h',
        '../../ash/wm/compact_status_area_layout_manager.cc',
        '../../ash/wm/compact_status_area_layout_manager.h',
        '../../ash/wm/default_container_event_filter.cc',
        '../../ash/wm/default_container_event_filter.h',
        '../../ash/wm/default_container_layout_manager.cc',
        '../../ash/wm/default_container_layout_manager.h',
        '../../ash/wm/image_grid.cc',
        '../../ash/wm/image_grid.h',
        '../../ash/wm/modal_container_layout_manager.cc',
        '../../ash/wm/modal_container_layout_manager.h',
        '../../ash/wm/modality_event_filter.cc',
        '../../ash/wm/modality_event_filter.h',
        '../../ash/wm/modality_event_filter_delegate.h',
        '../../ash/wm/property_util.cc',
        '../../ash/wm/property_util.h',
        '../../ash/wm/root_window_event_filter.cc',
        '../../ash/wm/root_window_event_filter.h',
        '../../ash/wm/root_window_layout_manager.cc',
        '../../ash/wm/root_window_layout_manager.h',
        '../../ash/wm/shadow.cc',
        '../../ash/wm/shadow.h',
        '../../ash/wm/shadow_controller.cc',
        '../../ash/wm/shadow_controller.h',
        '../../ash/wm/shadow_types.cc',
        '../../ash/wm/shadow_types.h',
        '../../ash/wm/shelf_layout_manager.cc',
        '../../ash/wm/shelf_layout_manager.h',
        '../../ash/wm/show_state_controller.h',
        '../../ash/wm/show_state_controller.cc',
        '../../ash/wm/stacking_controller.cc',
        '../../ash/wm/stacking_controller.h',
        '../../ash/wm/status_area_layout_manager.cc',
        '../../ash/wm/status_area_layout_manager.h',
        '../../ash/wm/toplevel_frame_view.cc',
        '../../ash/wm/toplevel_frame_view.h',
        '../../ash/wm/toplevel_layout_manager.cc',
        '../../ash/wm/toplevel_layout_manager.h',
        '../../ash/wm/toplevel_window_event_filter.cc',
        '../../ash/wm/toplevel_window_event_filter.h',
        '../../ash/wm/window_frame.cc',
        '../../ash/wm/window_frame.h',
        '../../ash/wm/window_properties.cc',
        '../../ash/wm/window_properties.h',
        '../../ash/wm/window_util.cc',
        '../../ash/wm/window_util.h',
        '../../ash/wm/workspace_controller.cc',
        '../../ash/wm/workspace_controller.h',
        '../../ash/wm/workspace/workspace.cc',
        '../../ash/wm/workspace/workspace.h',
        '../../ash/wm/workspace/workspace_manager.cc',
        '../../ash/wm/workspace/workspace_manager.h',
        '../../ash/wm/workspace/workspace_observer.h',
      ],
    },
    {
      'target_name': 'aura_shell_unittests',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:test_support_base',
        '../../chrome/chrome_resources.gyp:packed_resources',
        '../../build/temp_gyp/googleurl.gyp:googleurl',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../aura/aura.gyp:aura',
        '../aura/aura.gyp:test_support_aura',
        '../gfx/compositor/compositor.gyp:compositor_test_support',
        '../ui.gyp:gfx_resources',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
        '../ui.gyp:ui_resources_standard',
        '../views/views.gyp:views',
        'aura_shell',
      ],
      'sources': [
        'run_all_unittests.cc',
        'shell_unittest.cc',
        '../../ash/accelerators/accelerator_controller_unittest.cc',
        '../../ash/drag_drop/drag_drop_controller_unittest.cc',
        '../../ash/launcher/launcher_model_unittest.cc',
        '../../ash/launcher/launcher_unittest.cc',
        '../../ash/launcher/view_model_unittest.cc',
        '../../ash/launcher/view_model_utils_unittest.cc',
        '../../ash/test/test_suite.cc',
        '../../ash/test/test_suite.h',
        '../../ash/test/aura_shell_test_base.cc',
        '../../ash/test/aura_shell_test_base.h',
        '../../ash/test/test_activation_delegate.cc',
        '../../ash/test/test_activation_delegate.h',
        '../../ash/test/test_shell_delegate.cc',
        '../../ash/test/test_shell_delegate.h',
        '../../ash/tooltips/tooltip_controller_unittest.cc',
        '../../ash/wm/activation_controller_unittest.cc',
        '../../ash/wm/default_container_layout_manager_unittest.cc',
        '../../ash/wm/image_grid_unittest.cc',
        '../../ash/wm/modal_container_layout_manager_unittest.cc',
        '../../ash/wm/root_window_event_filter_unittest.cc',
        '../../ash/wm/shadow_controller_unittest.cc',
        '../../ash/wm/shelf_layout_manager_unittest.cc',
        '../../ash/wm/toplevel_layout_manager_unittest.cc',
        '../../ash/wm/toplevel_window_event_filter_unittest.cc',
        '../../ash/wm/workspace_controller_unittest.cc',
        '../../ash/wm/workspace/workspace_manager_unittest.cc',

        '<(SHARED_INTERMEDIATE_DIR)/ui/gfx/gfx_resources.rc',
        '<(SHARED_INTERMEDIATE_DIR)/ui/ui_resources/ui_resources.rc',
        '<(SHARED_INTERMEDIATE_DIR)/ui/ui_resources_standard/ui_resources_standard.rc',
      ],
      'conditions': [
        ['use_webkit_compositor==1', {
          'dependencies': [
            '../gfx/compositor/compositor.gyp:compositor',
          ],
        }, { # use_webkit_compositor!=1
          'dependencies': [
            '../gfx/compositor/compositor.gyp:test_compositor',
          ],
        }],
      ],
    },
    {
      'target_name': 'aura_shell_exe',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../chrome/chrome_resources.gyp:packed_resources',
        '../../skia/skia.gyp:skia',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../aura/aura.gyp:aura',
        '../gfx/compositor/compositor.gyp:compositor',
        '../gfx/compositor/compositor.gyp:compositor_test_support',
        '../ui.gyp:gfx_resources',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
        '../ui.gyp:ui_resources_standard',
        '../views/views.gyp:views',
        '../views/views.gyp:views_examples_lib',
        'aura_shell',
      ],
      'sources': [
        '../../ash/shell/app_list.cc',
        '../../ash/shell/bubble.cc',
        '../../ash/shell/example_factory.h',
        '../../ash/shell/lock_view.cc',
        '../../ash/shell/shell_main.cc',
        '../../ash/shell/toplevel_window.cc',
        '../../ash/shell/toplevel_window.h',
        '../../ash/shell/widgets.cc',
        '../../ash/shell/window_type_launcher.cc',
        '../../ash/shell/window_type_launcher.h',
        '<(SHARED_INTERMEDIATE_DIR)/ui/gfx/gfx_resources.rc',
        '<(SHARED_INTERMEDIATE_DIR)/ui/ui_resources_standard/ui_resources_standard.rc',
        '<(SHARED_INTERMEDIATE_DIR)/ui/ui_resources/ui_resources.rc',
        '../views/test/test_views_delegate.cc',
      ],
    },
  ],
}
