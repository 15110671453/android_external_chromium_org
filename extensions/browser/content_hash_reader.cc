// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/content_hash_reader.h"

#include "base/base64.h"
#include "base/file_util.h"
#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "crypto/sha2.h"
#include "extensions/browser/computed_hashes.h"
#include "extensions/browser/content_hash_tree.h"
#include "extensions/browser/verified_contents.h"
#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"

using base::DictionaryValue;
using base::ListValue;
using base::Value;

namespace extensions {

ContentHashReader::ContentHashReader(const std::string& extension_id,
                                     const base::Version& extension_version,
                                     const base::FilePath& extension_root,
                                     const base::FilePath& relative_path,
                                     const ContentVerifierKey& key)
    : extension_id_(extension_id),
      extension_version_(extension_version.GetString()),
      extension_root_(extension_root),
      relative_path_(relative_path),
      key_(key),
      status_(NOT_INITIALIZED),
      have_verified_contents_(false),
      have_computed_hashes_(false),
      block_size_(0) {
}

ContentHashReader::~ContentHashReader() {
}

bool ContentHashReader::Init() {
  DCHECK_EQ(status_, NOT_INITIALIZED);
  status_ = FAILURE;
  base::FilePath verified_contents_path =
      file_util::GetVerifiedContentsPath(extension_root_);

  if (!base::PathExists(verified_contents_path))
    return false;

  verified_contents_.reset(new VerifiedContents(key_.data, key_.size));
  if (!verified_contents_->InitFrom(verified_contents_path, false) ||
      !verified_contents_->valid_signature() ||
      !verified_contents_->version().Equals(extension_version_) ||
      verified_contents_->extension_id() != extension_id_)
    return false;

  have_verified_contents_ = true;

  base::FilePath computed_hashes_path =
      file_util::GetComputedHashesPath(extension_root_);
  if (!base::PathExists(computed_hashes_path))
    return false;

  ComputedHashes::Reader reader;
  if (!reader.InitFromFile(computed_hashes_path))
    return false;

  have_computed_hashes_ = true;

  if (!reader.GetHashes(relative_path_, &block_size_, &hashes_) ||
      block_size_ % crypto::kSHA256Length != 0)
    return false;

  const std::string* expected_root =
      verified_contents_->GetTreeHashRoot(relative_path_);
  if (!expected_root)
    return false;

  std::string root =
      ComputeTreeHashRoot(hashes_, block_size_ / crypto::kSHA256Length);
  if (*expected_root != root)
    return false;

  status_ = SUCCESS;
  return true;
}

int ContentHashReader::block_count() const {
  DCHECK(status_ != NOT_INITIALIZED);
  return hashes_.size();
}

int ContentHashReader::block_size() const {
  DCHECK(status_ != NOT_INITIALIZED);
  return block_size_;
}

bool ContentHashReader::GetHashForBlock(int block_index,
                                        const std::string** result) const {
  if (status_ != SUCCESS)
    return false;
  DCHECK(block_index >= 0);

  if (static_cast<unsigned>(block_index) >= hashes_.size())
    return false;
  *result = &hashes_[block_index];

  return true;
}

}  // namespace extensions
