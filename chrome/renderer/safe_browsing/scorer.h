// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This class loads a client-side model and lets you compute a phishing score
// for a set of previously extracted features.  The phishing score corresponds
// to the probability that the features are indicative of a phishing site.
//
// For more details on how the score is actually computed for a given model
// and a given set of features read the comments in client_model.proto file.
//
// See features.h for a list of features that are currently used.

#ifndef CHROME_RENDERER_SAFE_BROWSING_SCORER_H_
#define CHROME_RENDERER_SAFE_BROWSING_SCORER_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop_proxy.h"
#include "base/platform_file.h"
#include "base/string_piece.h"
#include "chrome/renderer/safe_browsing/client_model.pb.h"

namespace safe_browsing {
class FeatureMap;

// Scorer methods are virtual to simplify mocking of this class.
class Scorer {
 public:
  typedef Callback1<Scorer*>::Type CreationCallback;

  virtual ~Scorer();

  // Factory method which creates a new Scorer object by parsing the given
  // model.  If parsing fails this method returns NULL.
  static Scorer* Create(const base::StringPiece& model_str);

  // Factory method which creates a new Scorer object by asynchronously reading
  // a model from the given file.  file_thread_proxy should point to the thread
  // on which file I/O should take place.  On completion, creation_callback is
  // run with the new Scorer instance, or NULL if an error occurred.  Takes
  // ownership of creation_callback.  The creation callback will always run on
  // the same thread that called CreateFromFile.
  static void CreateFromFile(
      base::PlatformFile model_file,
      scoped_refptr<base::MessageLoopProxy> file_thread_proxy,
      CreationCallback* creation_callback);

  // This method computes the probability that the given features are indicative
  // of phishing.  It returns a score value that falls in the range [0.0,1.0]
  // (range is inclusive on both ends).
  virtual double ComputeScore(const FeatureMap& features) const;

  // -- Accessors used by the page feature extractor ---------------------------

  // Returns a set of hashed page terms that appear in the model in binary
  // format.
  const base::hash_set<std::string>& page_terms() const;

  // Returns a set of hashed page words that appear in the model in binary
  // format.
  const base::hash_set<std::string>& page_words() const;

  // Return the maximum number of words per term for the loaded model.
  size_t max_words_per_term() const;

  // The maximum size of a client-side phishing model file that we
  // expect to load.
  static const int kMaxPhishingModelSizeBytes;

 protected:
  // Most clients should use the factory method.  This constructor is public
  // to allow for mock implementations.
  Scorer();

 private:
  friend class PhishingScorerTest;

  // Computes the score for a given rule and feature map.  The score is computed
  // by multiplying the rule weight with the product of feature weights for the
  // given rule.  The feature weights are stored in the feature map.  If a
  // particular feature does not exist in the feature map we set its weight to
  // zero.
  double ComputeRuleScore(const ClientSideModel::Rule& rule,
                          const FeatureMap& features) const;

  ClientSideModel model_;
  base::hash_set<std::string> page_terms_;
  base::hash_set<std::string> page_words_;

  DISALLOW_COPY_AND_ASSIGN(Scorer);
};
}  // namepsace safe_browsing

#endif  // CHROME_RENDERER_SAFE_BROWSING_SCORER_H_
