// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/android/android_urls_sql_handler.h"

#include "base/logging.h"
#include "chrome/browser/history/history_database.h"

namespace history {

namespace {

// The interesting columns of this handler.
const HistoryAndBookmarkRow::ColumnID kInterestingColumns[] = {
    HistoryAndBookmarkRow::RAW_URL, HistoryAndBookmarkRow::URL_ID };

} // namespace

AndroidURLsSQLHandler::AndroidURLsSQLHandler(HistoryDatabase* history_db)
    : SQLHandler(kInterestingColumns, arraysize(kInterestingColumns)),
      history_db_(history_db) {
}

AndroidURLsSQLHandler::~AndroidURLsSQLHandler() {
}

bool AndroidURLsSQLHandler::Update(const HistoryAndBookmarkRow& row,
                                   const TableIDRows& ids_set) {
  DCHECK(row.is_value_set_explicitly(HistoryAndBookmarkRow::URL_ID));
  DCHECK(row.is_value_set_explicitly(HistoryAndBookmarkRow::RAW_URL));
  if (ids_set.size() != 1)
    return false;

  AndroidURLRow android_url_row;
  if (!history_db_->GetAndroidURLRow(ids_set[0].url_id, &android_url_row))
    return false;

  return history_db_->UpdateAndroidURLRow(android_url_row.id, row.raw_url(),
                                          row.url_id());
}

bool AndroidURLsSQLHandler::Insert(HistoryAndBookmarkRow* row) {
  AndroidURLID new_id = history_db_->AddAndroidURLRow(row->raw_url(),
                                                      row->url_id());
  row->set_id(new_id);
  return new_id;
}

bool AndroidURLsSQLHandler::Delete(const TableIDRows& ids_set) {
  std::vector<URLID> ids;
  for (TableIDRows::const_iterator id = ids_set.begin();
       id != ids_set.end(); ++id)
    ids.push_back(id->url_id);

  if (!ids.size())
    return true;

  return history_db_->DeleteAndroidURLRows(ids);
}

}  // namespace history.
