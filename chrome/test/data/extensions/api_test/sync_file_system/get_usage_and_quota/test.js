// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var testStep = [
  function () {
    chrome.syncFileSystem.requestFileSystem('drive', testStep.shift());
  },
  function(fs) {
    chrome.syncFileSystem.getUsageAndQuota(fs, testStep.shift());
  },
  function(info) {
    chrome.test.assertEq(0, info.usage_bytes);

    // TODO(calvinlo): Update test code after default quota is made const
    // (http://crbug.com/155488).
    chrome.test.assertEq(123456789, info.quota_bytes);
    chrome.test.succeed();
  }
];

function errorHandler() {
  chrome.test.fail();
}

chrome.test.runTests([
  testStep[0]
]);
