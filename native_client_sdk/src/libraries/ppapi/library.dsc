{
  'TOOLS': ['win', 'linux', 'mac'],
  'SEARCH': [
      '.',
      '../../../../ppapi/c',
      '../../../../ppapi/c/dev',
      '../../../../ppapi/c/extensions/dev',
  ],
  'TARGETS': [
    {
      'NAME' : 'ppapi',
      'TYPE' : 'lib',
      'SOURCES' : [
        'ppapi_externs.c'
      ],
    }
  ],
  'HEADERS': [
    {
      'FILES': [
        'pp_array_output.h',
        'ppb_audio_buffer.h',
        'ppb_audio_config.h',
        'ppb_audio.h',
        'ppb_console.h',
        'ppb_core.h',
        'ppb_file_io.h',
        'ppb_file_mapping.h',
        'ppb_file_ref.h',
        'ppb_file_system.h',
        'ppb_fullscreen.h',
        'ppb_gamepad.h',
        'ppb_graphics_2d.h',
        'ppb_graphics_3d.h',
        'ppb.h',
        'ppb_host_resolver.h',
        'ppb_image_data.h',
        'ppb_input_event.h',
        'ppb_instance.h',
        'ppb_media_stream_audio_track.h',
        'ppb_media_stream_video_track.h',
        'ppb_message_loop.h',
        'ppb_messaging.h',
        'ppb_mouse_cursor.h',
        'ppb_mouse_lock.h',
        'ppb_net_address.h',
        'ppb_network_list.h',
        'ppb_network_monitor.h',
        'ppb_network_proxy.h',
        'pp_bool.h',
        'ppb_opengles2.h',
        'ppb_tcp_socket.h',
        'ppb_text_input_controller.h',
        'ppb_udp_socket.h',
        'ppb_url_loader.h',
        'ppb_url_request_info.h',
        'ppb_url_response_info.h',
        'ppb_var_array_buffer.h',
        'ppb_var_array.h',
        'ppb_var_dictionary.h',
        'ppb_var.h',
        'ppb_video_decoder.h',
        'ppb_video_frame.h',
        'ppb_view.h',
        'ppb_websocket.h',
        'pp_codecs.h',
        'pp_completion_callback.h',
        'pp_directory_entry.h',
        'pp_errors.h',
        'pp_file_info.h',
        'pp_graphics_3d.h',
        'pp_input_event.h',
        'pp_instance.h',
        'pp_macros.h',
        'pp_module.h',
        'ppp_graphics_3d.h',
        'ppp.h',
        'ppp_input_event.h',
        'ppp_instance.h',
        'ppp_message_handler.h',
        'ppp_messaging.h',
        'ppp_mouse_lock.h',
        'pp_point.h',
        'pp_rect.h',
        'pp_resource.h',
        'pp_size.h',
        'pp_stdint.h',
        'pp_time.h',
        'pp_touch_point.h',
        'pp_var.h',
      ],
      'DEST': 'include/ppapi/c',
    },
    {
      'FILES': [
        'deprecated_bool.h',
        'ppb_alarms_dev.h',
        'ppb_cursor_control_dev.h',
        'ppb_file_chooser_dev.h',
        'ppb_font_dev.h',
        'ppb_memory_dev.h',
        'ppb_opengles2ext_dev.h',
        'ppb_printing_dev.h',
        'ppb_trace_event_dev.h',
        'ppb_truetype_font_dev.h',
        'ppb_var_deprecated.h',
        'ppb_view_dev.h',
        'ppb_zoom_dev.h',
        'pp_cursor_type_dev.h',
        'pp_optional_structs_dev.h',
        'ppp_class_deprecated.h',
        'ppp_network_state_dev.h',
        'ppp_printing_dev.h',
        'pp_print_settings_dev.h',
        'ppp_scrollbar_dev.h',
        'ppp_selection_dev.h',
        'ppp_text_input_dev.h',
        'ppp_zoom_dev.h',
      ],
      'DEST': 'include/ppapi/c/dev',
    },
  ],
  'DEST': 'src',
  'NAME': 'ppapi',
}

