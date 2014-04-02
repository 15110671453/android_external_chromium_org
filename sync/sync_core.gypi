# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'include_dirs': [
    '..',
  ],
  'defines': [
    'SYNC_IMPLEMENTATION',
  ],
  'dependencies': [
    '../base/base.gyp:base',
    '../base/base.gyp:base_i18n',
    '../crypto/crypto.gyp:crypto',
    '../google_apis/google_apis.gyp:google_apis',
    '../net/net.gyp:net',
    '../sql/sql.gyp:sql',
    '../url/url.gyp:url_lib',
  ],
  'conditions': [
    ['OS=="linux" and chromeos==1', {
      # Required by get_session_name.cc on Chrome OS.
      'dependencies': [
        '../chromeos/chromeos.gyp:chromeos',
      ],
    }],
  ],
  'sources': [
    'base/sync_export.h',
    'engine/all_status.cc',
    'engine/all_status.h',
    'engine/apply_control_data_updates.cc',
    'engine/apply_control_data_updates.h',
    'engine/backoff_delay_provider.cc',
    'engine/backoff_delay_provider.h',
    'engine/commit.cc',
    'engine/commit_contribution.cc',
    'engine/commit_contribution.h',
    'engine/commit_contributor.cc',
    'engine/commit_contributor.h',
    'engine/commit.h',
    'engine/commit_processor.cc',
    'engine/commit_processor.h',
    'engine/commit_util.cc',
    'engine/commit_util.h',
    'engine/conflict_resolver.cc',
    'engine/conflict_resolver.h',
    'engine/conflict_util.cc',
    'engine/conflict_util.h',
    'engine/directory_commit_contribution.cc',
    'engine/directory_commit_contribution.h',
    'engine/directory_commit_contributor.cc',
    'engine/directory_commit_contributor.h',
    'engine/directory_update_handler.cc',
    'engine/directory_update_handler.h',
    'engine/get_commit_ids.cc',
    'engine/get_commit_ids.h',
    'engine/get_updates_delegate.cc',
    'engine/get_updates_delegate.h',
    'engine/get_updates_processor.cc',
    'engine/get_updates_processor.h',
    'engine/net/server_connection_manager.cc',
    'engine/net/server_connection_manager.h',
    'engine/net/url_translator.cc',
    'engine/net/url_translator.h',
    'engine/non_blocking_type_processor_core.cc',
    'engine/non_blocking_type_processor_core.h',
    'engine/nudge_source.cc',
    'engine/nudge_source.h',
    'engine/process_updates_util.cc',
    'engine/process_updates_util.h',
    'engine/sync_cycle_event.cc',
    'engine/sync_cycle_event.h',
    'engine/sync_engine_event_listener.cc',
    'engine/sync_engine_event_listener.h',
    'engine/syncer.cc',
    'engine/syncer.h',
    'engine/syncer_proto_util.cc',
    'engine/syncer_proto_util.h',
    'engine/syncer_types.h',
    'engine/syncer_util.cc',
    'engine/syncer_util.h',
    'engine/sync_scheduler.cc',
    'engine/sync_scheduler.h',
    'engine/sync_scheduler_impl.cc',
    'engine/sync_scheduler_impl.h',
    'engine/traffic_logger.cc',
    'engine/traffic_logger.h',
    'engine/update_applicator.cc',
    'engine/update_applicator.h',
    'engine/update_handler.cc',
    'engine/update_handler.h',
    'js/js_arg_list.cc',
    'js/js_arg_list.h',
    'js/js_backend.h',
    'js/js_controller.h',
    'js/js_event_details.cc',
    'js/js_event_details.h',
    'js/js_event_handler.h',
    'js/js_reply_handler.h',
    'js/sync_js_controller.cc',
    'js/sync_js_controller.h',
    'protocol/proto_enum_conversions.cc',
    'protocol/proto_enum_conversions.h',
    'protocol/proto_value_conversions.cc',
    'protocol/proto_value_conversions.h',
    'protocol/sync_protocol_error.cc',
    'protocol/sync_protocol_error.h',
    'sessions/data_type_tracker.cc',
    'sessions/data_type_tracker.h',
    'sessions/debug_info_getter.h',
    'sessions/model_type_registry.cc',
    'sessions/model_type_registry.h',
    'sessions/nudge_tracker.cc',
    'sessions/nudge_tracker.h',
    'sessions/status_controller.cc',
    'sessions/status_controller.h',
    'sessions/sync_session.cc',
    'sessions/sync_session.h',
    'sessions/sync_session_context.cc',
    'sessions/sync_session_context.h',
    'syncable/blob.h',
    'syncable/dir_open_result.h',
    'syncable/directory.cc',
    'syncable/directory.h',
    'syncable/directory_backing_store.cc',
    'syncable/directory_backing_store.h',
    'syncable/directory_change_delegate.h',
    'syncable/entry.cc',
    'syncable/entry.h',
    'syncable/entry_kernel.cc',
    'syncable/entry_kernel.h',
    'syncable/in_memory_directory_backing_store.cc',
    'syncable/in_memory_directory_backing_store.h',
    'syncable/invalid_directory_backing_store.cc',
    'syncable/invalid_directory_backing_store.h',
    'syncable/metahandle_set.h',
    'syncable/model_neutral_mutable_entry.cc',
    'syncable/model_neutral_mutable_entry.h',
    'syncable/model_type.cc',
    'syncable/mutable_entry.cc',
    'syncable/mutable_entry.h',
    'syncable/nigori_handler.cc',
    'syncable/nigori_handler.h',
    'syncable/nigori_util.cc',
    'syncable/nigori_util.h',
    'syncable/on_disk_directory_backing_store.cc',
    'syncable/on_disk_directory_backing_store.h',
    'syncable/parent_child_index.cc',
    'syncable/parent_child_index.h',
    'syncable/scoped_kernel_lock.cc',
    'syncable/scoped_kernel_lock.h',
    'syncable/scoped_parent_child_index_updater.cc',
    'syncable/scoped_parent_child_index_updater.h',
    'syncable/syncable-inl.h',
    'syncable/syncable_base_transaction.cc',
    'syncable/syncable_base_transaction.h',
    'syncable/syncable_base_write_transaction.cc',
    'syncable/syncable_base_write_transaction.h',
    'syncable/syncable_changes_version.h',
    'syncable/syncable_columns.h',
    'syncable/syncable_delete_journal.cc',
    'syncable/syncable_delete_journal.h',
    'syncable/syncable_enum_conversions.cc',
    'syncable/syncable_enum_conversions.h',
    'syncable/syncable_id.cc',
    'syncable/syncable_id.h',
    'syncable/syncable_model_neutral_write_transaction.cc',
    'syncable/syncable_model_neutral_write_transaction.h',
    'syncable/syncable_proto_util.cc',
    'syncable/syncable_proto_util.h',
    'syncable/syncable_read_transaction.cc',
    'syncable/syncable_read_transaction.h',
    'syncable/syncable_util.cc',
    'syncable/syncable_util.h',
    'syncable/syncable_write_transaction.cc',
    'syncable/syncable_write_transaction.h',
    'syncable/transaction_observer.h',
    'syncable/write_transaction_info.cc',
    'syncable/write_transaction_info.h',
    'util/cryptographer.cc',
    'util/cryptographer.h',
    'util/data_type_histogram.h',
    'util/encryptor.h',
    'util/extensions_activity.cc',
    'util/extensions_activity.h',
    'util/get_session_name.cc',
    'util/get_session_name.h',
    'util/get_session_name_ios.mm',
    'util/get_session_name_ios.h',
    'util/get_session_name_linux.cc',
    'util/get_session_name_linux.h',
    'util/get_session_name_mac.mm',
    'util/get_session_name_mac.h',
    'util/get_session_name_win.cc',
    'util/get_session_name_win.h',
    'util/logging.cc',
    'util/logging.h',
    'util/nigori.cc',
    'util/nigori.h',
    'util/time.cc',
    'util/time.h',
  ],
}
