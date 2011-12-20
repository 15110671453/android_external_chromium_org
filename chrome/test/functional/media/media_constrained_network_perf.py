#!/usr/bin/env python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Records metrics on playing media under constrained network conditions.

Spins up a Constrained Network Server (CNS) and runs through a test matrix of
bandwidth, latency, and packet loss settings.  Each run records a
time-to-playback (TTP) and extra-play-percentage (EPP) metric in a format
consumable by the Chromium perf bots.

Since even a small number of different settings yields a large test matrix, the
design is threaded... however PyAuto is not, so a global lock is used when calls
into PyAuto are necessary.  The number of threads can be set by _TEST_THREADS.

The CNS code is located under: <root>/src/media/tools/constrained_network_server
"""

import itertools
import os
import Queue
import subprocess
import sys
import threading

import pyauto_media
import pyauto
import pyauto_paths
import pyauto_utils


# Settings for each network constraint.
_BANDWIDTH_SETTINGS_KBPS = {'None': 0, 'Low': 256, 'Medium': 2000, 'High': 5000}
_LATENCY_SETTINGS_MS = {'None': 0, 'Low': 43, 'Medium': 105, 'High': 180}
_PACKET_LOSS_SETTINGS_PERCENT = {'None': 0, 'Medium': 2, 'High': 5}

# Test constraints are all possible combination of the above settings.  Each
# tuple must be of the form (Bandwidth, Latency, Packet Loss).
_TEST_CONSTRAINTS = itertools.product(
    _BANDWIDTH_SETTINGS_KBPS.values(),
    _LATENCY_SETTINGS_MS.values(),
    _PACKET_LOSS_SETTINGS_PERCENT.values())

_TEST_CONSTRAINT_NAMES = itertools.product(
    _BANDWIDTH_SETTINGS_KBPS.keys(),
    _LATENCY_SETTINGS_MS.keys(),
    _PACKET_LOSS_SETTINGS_PERCENT.keys())

# HTML test path; relative to src/chrome/test/data.  Loads a test video and
# records metrics in JavaScript.
_TEST_HTML_PATH = os.path.join(
    'media', 'html', 'media_constrained_network.html')

# Number of threads to use during testing.
_TEST_THREADS = 3

# File name of video to collect metrics for and its duration (used for timeout).
# TODO(dalecurtis): Should be set on the command line.
_TEST_VIDEO = 'roller.webm'
_TEST_VIDEO_DURATION_SEC = 28.53


# Path to CNS executable relative to source root.
_CNS_PATH = os.path.join(
    'media', 'tools', 'constrained_network_server', 'cns.py')

# Port to start the CNS on.
_CNS_PORT = 9000

# Base CNS URL, only requires & separated parameter names appended.
_CNS_BASE_URL = 'http://127.0.0.1:%d/ServeConstrained?' % _CNS_PORT


class TestWorker(threading.Thread):
  """Worker thread.  For each queue entry: opens tab, runs test, closes tab."""

  # Atomic, monotonically increasing task identifier.  Used to ID tabs.
  _task_id = itertools.count()

  def __init__(self, pyauto_test, tasks, automation_lock, url):
    """Sets up TestWorker class variables.

    Args:
      pyauto_test: Reference to a pyauto.PyUITest instance.
      tasks: Queue containing (settings, name) tuples.
      automation_lock: Global automation lock for pyauto calls.
      url: File URL to HTML/JavaScript test code.
    """
    threading.Thread.__init__(self)
    self._tasks = tasks
    self._automation_lock = automation_lock
    self._pyauto = pyauto_test
    self._url = url
    self.start()

  def _FindTabLocked(self, url):
    """Returns the tab index for the tab belonging to this url.

    self._automation_lock must be owned by caller.
    """
    for tab in self._pyauto.GetBrowserInfo()['windows'][0]['tabs']:
      if tab['url'] == url:
        return tab['index']

  def _HaveMetrics(self, unique_url):
    """Returns true if metrics are ready.  Set self.{_epp,_ttp} < 0 pre-run."""
    with self._automation_lock:
      tab = self._FindTabLocked(unique_url)

      if self._epp < 0:
        self._epp = int(self._pyauto.GetDOMValue('extra_play_percentage',
                                                 tab_index=tab))
      if self._ttp < 0:
        self._ttp = int(self._pyauto.GetDOMValue('time_to_playback',
                                                 tab_index=tab))
    return self._epp >= 0 and self._ttp >= 0

  def run(self):
    """Opens tab, starts HTML test, and records metrics for each queue entry.

    No exception handling is done to make sure the main thread exits properly
    during Chrome crashes or other failures.  Doing otherwise has the potential
    to leave the CNS server running in the background.

    For a clean shutdown, put the magic exit value (None, None) in the queue.
    """
    while True:
      settings, name = self._tasks.get()

      # Check for magic exit values.
      if (settings, name) == (None, None):
        break

      # Build video source URL.  Values <= 0 mean the setting is disabled.
      video_url = [_CNS_BASE_URL, 'f=' + _TEST_VIDEO]
      if settings[0] > 0:
        video_url.append('bandwidth=%d' % settings[0])
      if settings[1] > 0:
        video_url.append('latency=%d' % settings[1])
      if settings[2] > 0:
        video_url.append('loss=%d' % settings[2])
      video_url = '&'.join(video_url)

      # Make the test URL unique so we can figure out our tab index later.
      unique_url = '%s?%d' % (self._url, TestWorker._task_id.next())

      # Start the test!
      with self._automation_lock:
        self._pyauto.AppendTab(pyauto.GURL(unique_url))
        self._pyauto.CallJavascriptFunc(
            'startTest', [video_url], tab_index=self._FindTabLocked(unique_url))

      # Wait until the necessary metrics have been collected.  Okay to not lock
      # here since pyauto.WaitUntil doesn't call into Chrome.
      self._epp = self._ttp = -1
      self._pyauto.WaitUntil(
        self._HaveMetrics, args=[unique_url], retry_sleep=2,
        timeout=_TEST_VIDEO_DURATION_SEC * 10)

      # Record results.
      # TODO(dalecurtis): Support reference builds.
      series_name = ''.join(name)
      pyauto_utils.PrintPerfResult('epp', series_name, self._epp, '%')
      pyauto_utils.PrintPerfResult('ttp', series_name, self._ttp, 'ms')

      # Close the tab.
      with self._automation_lock:
        self._pyauto.GetBrowserWindow(0).GetTab(
            self._FindTabLocked(unique_url)).Close(True)

      # TODO(dalecurtis): Check results for regressions.
      self._tasks.task_done()


class MediaConstrainedNetworkPerfTest(pyauto.PyUITest):
  """PyAuto test container.  See file doc string for more information."""

  def setUp(self):
    """Starts the Constrained Network Server (CNS)."""
    cmd = [sys.executable, os.path.join(pyauto_paths.GetSourceDir(), _CNS_PATH),
           '--port', str(_CNS_PORT),
           '--interface', 'lo',
           '--www-root', os.path.join(
               self.DataDir(), 'pyauto_private', 'media')]

    process = subprocess.Popen(cmd, stderr=subprocess.PIPE)

    # Wait for server to start up.
    line = True
    while line:
      line = process.stderr.readline()
      if 'STARTED' in line:
        self._server_pid = process.pid
        pyauto.PyUITest.setUp(self)
        return
    self.fail('Failed to start CNS.')

  def tearDown(self):
    """Stops the Constrained Network Server (CNS)."""
    pyauto.PyUITest.tearDown(self)
    self.Kill(self._server_pid)

  def testConstrainedNetworkPerf(self):
    """Starts CNS, spins up worker threads to run through _TEST_CONSTRAINTS."""
    # Convert relative test path into an absolute path.
    test_url = self.GetFileURLForDataPath(_TEST_HTML_PATH)

    # PyAuto doesn't support threads, so we synchronize all automation calls.
    automation_lock = threading.Lock()

    # Spin up worker threads.
    tasks = Queue.Queue()
    threads = []
    for _ in xrange(_TEST_THREADS):
      threads.append(TestWorker(self, tasks, automation_lock, test_url))

    for settings, name in zip(_TEST_CONSTRAINTS, _TEST_CONSTRAINT_NAMES):
      tasks.put((settings, name))

    # Add shutdown magic to end of queue.
    for thread in threads:
      tasks.put((None, None))

    # Wait for threads to exit, gracefully or otherwise.
    for thread in threads:
      thread.join()


if __name__ == '__main__':
  # TODO(dalecurtis): Process command line parameters here.
  pyauto_media.Main()
