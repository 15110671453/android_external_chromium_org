// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPELLCHECKER_SPELLCHECK_HOST_METRICS_H_
#define CHROME_BROWSER_SPELLCHECKER_SPELLCHECK_HOST_METRICS_H_

#include <string>
#include <vector>

#include "base/hash_tables.h"
#include "base/time.h"
#include "base/timer.h"

// A helper object for recording spell-check related histograms.
// This class encapsulates histogram names and metrics API.
// This also carries a set of counters for collecting histograms
// and a timer for making a periodical summary.
//
// We expect a user of SpellCheckHost class to instantiate this object,
// and pass the metrics object to SpellCheckHost's factory method.
//
//   metrics.reset(new SpellCheckHostMetrics());
//   spell_check_host = SpellChecHost::Create(...., metrics.get());
//
// The lifetime of the object should be managed by a caller,
// and the lifetime should be longer than SpellCheckHost instance
// because SpellCheckHost will use the object.
class SpellCheckHostMetrics {
 public:
  SpellCheckHostMetrics();
  ~SpellCheckHostMetrics();

  // Collects the number of words in the custom dictionary, which is
  // to be uploaded via UMA.
  void RecordCustomWordCountStats(size_t count);

  // Collects status of spellchecking enabling state, which is
  // to be uploaded via UMA
  void RecordEnabledStats(bool enabled);

  // Collects a histogram for dictionary corruption rate
  // to be uploaded via UMA
  void RecordDictionaryCorruptionStats(bool corrupted);

  // Collects status of spellchecking enabling state, which is
  // to be uploaded via UMA
  void RecordCheckedWordStats(const string16& word, bool misspell);

  // Collects a histogram for misspelled word replacement
  // to be uploaded via UMA
  void RecordReplacedWordStats(int delta);

  // Collects a histogram for context menu showing as a spell correction
  // attempt to be uploaded via UMA
  void RecordSuggestionStats(int delta);

 private:
  friend class SpellcheckHostMetricsTest;
  void OnHistogramTimerExpired();

  // Records various counters without changing their values.
  void RecordWordCounts();

  // Number of corrected words of checked words.
  int misspelled_word_count_;
  int last_misspelled_word_count_;

  // Number of checked words.
  int spellchecked_word_count_;
  int last_spellchecked_word_count_;

  // Number of suggestion list showings.
  int suggestion_show_count_;
  int last_suggestion_show_count_;

  // Number of misspelled words replaced by a user.
  int replaced_word_count_;
  int last_replaced_word_count_;

  // Last recorded number of unique words.
  int last_unique_word_count_;

  // Time when first spellcheck happened.
  base::TimeTicks start_time_;
  // Set of checked words in the hashed form.
  base::hash_set<std::string> checked_word_hashes_;
  base::RepeatingTimer<SpellCheckHostMetrics> recording_timer_;
};

#endif  // CHROME_BROWSER_SPELLCHECKER_SPELLCHECK_HOST_METRICS_H_
