// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.onload = function() {
  var patterns = [ "http://*.google.com/*" ];
  chrome.contextMenus.create({"title":"item1", "contexts": ["all"],
                              "targetUrlPatterns": patterns}, function() {
    if (!chrome.extension.lastError) {
      chrome.test.sendMessage("created items");
    }
  });
};
