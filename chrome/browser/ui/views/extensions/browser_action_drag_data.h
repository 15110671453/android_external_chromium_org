// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXTENSIONS_BROWSER_ACTION_DRAG_DATA_H_
#define CHROME_BROWSER_UI_VIEWS_EXTENSIONS_BROWSER_ACTION_DRAG_DATA_H_

#include <string>

#include "base/basictypes.h"

#if defined(TOOLKIT_VIEWS)
#include "ui/base/dragdrop/os_exchange_data.h"
#endif

class Pickle;
class Profile;

class BrowserActionDragData {
 public:
  BrowserActionDragData();
  BrowserActionDragData(const std::string& id, int index);

  // These mirror the views::View and views::MenuDelegate methods for dropping,
  // and return the appropriate results for being able to drop an extension's
  // BrowserAction view.
  static bool GetDropFormats(
      std::set<ui::OSExchangeData::CustomFormat>* custom_formats);
  static bool AreDropTypesRequired();
  static bool CanDrop(const ui::OSExchangeData& data, const Profile* profile);

  const std::string& id() const { return id_; }

  size_t index() const { return index_; }

  // Returns true if this data is from the specified profile.
  bool IsFromProfile(const Profile* profile) const;

#if defined(TOOLKIT_VIEWS)
  void Write(Profile* profile, ui::OSExchangeData* data) const;

  // Restores this data from the clipboard, returning true on success.
  bool Read(const ui::OSExchangeData& data);

  // Returns the Custom Format this class supports (for Browser Actions).
  static const ui::OSExchangeData::CustomFormat& GetBrowserActionCustomFormat();
#endif

 private:
  void WriteToPickle(Profile* profile, Pickle* pickle) const;
  bool ReadFromPickle(Pickle* pickle);

  // The profile we originated from.
  void* profile_;

  // The id of the view being dragged.
  std::string id_;

  // The index of the view being dragged.
  size_t index_;

  DISALLOW_COPY_AND_ASSIGN(BrowserActionDragData);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXTENSIONS_BROWSER_ACTION_DRAG_DATA_H_
