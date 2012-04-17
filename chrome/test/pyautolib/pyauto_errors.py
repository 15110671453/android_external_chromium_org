# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""PyAuto Errors."""

class JavascriptRuntimeError(RuntimeError):
  """Represent an error raised by injected Javascript."""
  pass

class JSONInterfaceError(RuntimeError):
  """Represent an error in the JSON ipc interface."""
  pass

class NTPThumbnailNotShownError(RuntimeError):
  """Represent an error from attempting to manipulate a NTP thumbnail that
  is not visible to a real user."""
  pass
