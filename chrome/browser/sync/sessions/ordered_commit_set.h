// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_SESSIONS_ORDERED_COMMIT_SET_H_
#define CHROME_BROWSER_SYNC_SESSIONS_ORDERED_COMMIT_SET_H_
#pragma once

#include <map>
#include <set>
#include <vector>

#include "chrome/browser/sync/engine/model_safe_worker.h"
#include "chrome/browser/sync/syncable/model_type.h"
#include "chrome/browser/sync/syncable/syncable_id.h"

namespace browser_sync {
namespace sessions {

// TODO(ncarter): This code is more generic than just Commit and can
// be reused elsewhere (e.g. ChangeReorderBuffer do similar things).  Merge
// all these implementations.
class OrderedCommitSet {
 public:
  // A list of indices into the full list of commit ids such that:
  // 1 - each element is an index belonging to a particular ModelSafeGroup.
  // 2 - the vector is in sorted (smallest to largest) order.
  // 3 - each element is a valid index for GetCommitItemAt.
  // See GetCommitIdProjection for usage.
  typedef std::vector<size_t> Projection;

  // TODO(chron): Reserve space according to batch size?
  explicit OrderedCommitSet(const browser_sync::ModelSafeRoutingInfo& routes);
  ~OrderedCommitSet();

  bool HaveCommitItem(const int64 metahandle) const {
    return inserted_metahandles_.count(metahandle) > 0;
  }

  void AddCommitItem(const int64 metahandle, const syncable::Id& commit_id,
                     syncable::ModelType type);

  const std::vector<syncable::Id>& GetAllCommitIds() const {
    return commit_ids_;
  }

  // Return the Id at index |position| in this OrderedCommitSet.  Note that
  // the index uniquely identifies the same logical item in each of:
  // 1) this OrderedCommitSet
  // 2) the CommitRequest sent to the server
  // 3) the list of EntryResponse objects in the CommitResponse.
  // These together allow re-association of the pre-commit Id with the
  // actual committed entry.
  const syncable::Id& GetCommitIdAt(const size_t position) const {
    return commit_ids_[position];
  }

  // Same as above, but for ModelType of the item.
  syncable::ModelType GetModelTypeAt(const size_t position) const {
    return types_[position];
  }

  // Get the projection of commit ids onto the space of commit ids
  // belonging to |group|.  This is useful when you need to process a commit
  // response one ModelSafeGroup at a time. See GetCommitIdAt for how the
  // indices contained in the returned Projection can be used.
  const Projection& GetCommitIdProjection(browser_sync::ModelSafeGroup group) {
    return projections_[group];
  }

  int Size() const {
    return commit_ids_.size();
  }

  // Returns true iff any of the commit ids added to this set have model type
  // BOOKMARKS.
  bool HasBookmarkCommitId() const;

  void Append(const OrderedCommitSet& other);
  void AppendReverse(const OrderedCommitSet& other);
  void Truncate(size_t max_size);

  void operator=(const OrderedCommitSet& other);
 private:
  // A set of CommitIdProjections associated with particular ModelSafeGroups.
  typedef std::map<browser_sync::ModelSafeGroup, Projection> Projections;

  // Helper container for return value of GetCommitItemAt.
  struct CommitItem {
    int64 meta;
    syncable::Id id;
    syncable::ModelType group;
  };

  CommitItem GetCommitItemAt(const int position) const;

  // These lists are different views of the same items; e.g they are
  // isomorphic.
  std::set<int64> inserted_metahandles_;
  std::vector<syncable::Id> commit_ids_;
  std::vector<int64> metahandle_order_;
  Projections projections_;

  // We need this because of operations like AppendReverse that take ids from
  // one OrderedCommitSet and insert into another -- we need to know the
  // group for each ID so that the insertion can update the appropriate
  // projection.  We could store it in commit_ids_, but sometimes we want
  // to just return the vector of Ids, so this is more straightforward
  // and shouldn't take up too much extra space since commit lists are small.
  std::vector<syncable::ModelType> types_;

  browser_sync::ModelSafeRoutingInfo routes_;
};

}  // namespace sessions
}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_SESSIONS_ORDERED_COMMIT_SET_H_

