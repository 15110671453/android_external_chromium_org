#!/usr/bin/env python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs all the native unit tests.

1. Copy over test binary to /data/local on device.
2. Resources: chrome/unit_tests requires resources (chrome.pak and en-US.pak)
   to be deployed to the device (in /data/local/tmp).
3. Environment:
3.1. chrome/unit_tests requires (via chrome_paths.cc) a directory named:
     /data/local/tmp/chrome/test/data
3.2. page_cycler_tests have following requirements,
3.2.1  the following data on host:
       <chrome_src_dir>/tools/page_cycler
       <chrome_src_dir>/data/page_cycler
3.2.2. two data directories to store above test data on device named:
       /data/local/tmp/tools/ (for database perf test)
       /data/local/tmp/data/ (for other perf tests)
3.2.3. a http server to serve http perf tests.
       The http root is host's <chrome_src_dir>/data/page_cycler/, port 8000.
3.2.4  a tool named forwarder is also required to run on device to
       forward the http request/response between host and device.
3.2.5  Chrome is installed on device.
4. Run the binary in the device and stream the log to the host.
4.1. Optionally, filter specific tests.
4.2. Optionally, rebaseline: run the available tests and update the
     suppressions file for failures.
4.3. If we're running a single test suite and we have multiple devices
     connected, we'll shard the tests.
5. Clean up the device.

Suppressions:

Individual tests in a test binary can be suppressed by listing it in
the gtest_filter directory in a file of the same name as the test binary,
one test per line. Here is an example:

  $ cat gtest_filter/base_unittests_disabled
  DataPackTest.Load
  ReadOnlyFileUtilTest.ContentsEqual

This file is generated by the tests running on devices. If running on emulator,
additonal filter file which lists the tests only failed in emulator will be
loaded. We don't care about the rare testcases which succeeded on emuatlor, but
failed on device.
"""

import logging
import os
import re
import subprocess
import sys
import time

import android_commands
import cmd_helper
import debug_info
import emulator
import run_tests_helper
from single_test_runner import SingleTestRunner
from test_package_executable import TestPackageExecutable
from test_result import BaseTestResult, TestResults

_TEST_SUITES = ['base_unittests', 'sql_unittests', 'ipc_tests', 'net_unittests']

class Xvfb(object):
  """Class to start and stop Xvfb if relevant.  Nop if not Linux."""

  def __init__(self):
    self._pid = 0

  def _IsLinux(self):
    """Return True if on Linux; else False."""
    return sys.platform.startswith('linux')

  def Start(self):
    """Start Xvfb and set an appropriate DISPLAY environment.  Linux only.

    Copied from tools/code_coverage/coverage_posix.py
    """
    if not self._IsLinux():
      return
    proc = subprocess.Popen(["Xvfb", ":9", "-screen", "0", "1024x768x24",
                             "-ac"],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    self._pid = proc.pid
    if not self._pid:
      raise Exception('Could not start Xvfb')
    os.environ['DISPLAY'] = ":9"

    # Now confirm, giving a chance for it to start if needed.
    for test in range(10):
      proc = subprocess.Popen('xdpyinfo >/dev/null', shell=True)
      pid, retcode = os.waitpid(proc.pid, 0)
      if retcode == 0:
        break
      time.sleep(0.25)
    if retcode != 0:
      raise Exception('Could not confirm Xvfb happiness')

  def Stop(self):
    """Stop Xvfb if needed.  Linux only."""
    if self._pid:
      try:
        os.kill(self._pid, signal.SIGKILL)
      except:
        pass
      del os.environ['DISPLAY']
      self._pid = 0


def RunTests(device, test_suite, gtest_filter, test_arguments, rebaseline,
             timeout, performance_test, cleanup_test_files, tool,
             log_dump_name):
  """Runs the tests.

  Args:
    device: Device to run the tests.
    test_suite: A specific test suite to run, empty to run all.
    gtest_filter: A gtest_filter flag.
    test_arguments: Additional arguments to pass to the test binary.
    rebaseline: Whether or not to run tests in isolation and update the filter.
    timeout: Timeout for each test.
    performance_test: Whether or not performance test(s).
    cleanup_test_files: Whether or not to cleanup test files on device.
    tool: Name of the Valgrind tool.
    log_dump_name: Name of log dump file.

  Returns:
    A TestResults object.
  """
  results = []

  if test_suite:
    global _TEST_SUITES
    if not os.path.exists(test_suite):
      logging.critical('Unrecognized test suite, supported: %s' %
                       _TEST_SUITES)
      if test_suite in _TEST_SUITES:
        logging.critical('(Remember to include the path: out/Release/%s)',
                         test_suite)
      return TestResults.FromOkAndFailed([], [BaseTestResult(test_suite, '')])
    _TEST_SUITES = [test_suite]
  else:
    # If not specified, assume the test suites are in out/Release
    test_suite_dir = os.path.abspath(os.path.join(run_tests_helper.CHROME_DIR,
        'out', 'Release'))
    _TEST_SUITES = [os.path.join(test_suite_dir, t) for t in _TEST_SUITES]
  debug_info_list = []
  print _TEST_SUITES  # So it shows up in buildbot output
  for t in _TEST_SUITES:
    test = SingleTestRunner(device, t, gtest_filter, test_arguments,
                            timeout, rebaseline, performance_test,
                            cleanup_test_files, tool, not not log_dump_name)
    test.RunTests()
    results += [test.test_results]
    # Collect debug info.
    debug_info_list += [test.dump_debug_info]
    if rebaseline:
      test.UpdateFilter(test.test_results.failed)
    elif test.test_results.failed:
      # Stop running test if encountering failed test.
      test.test_results.LogFull()
      break
  # Zip all debug info outputs into a file named by log_dump_name.
  debug_info.GTestDebugInfo.ZipAndCleanResults(
      os.path.join(run_tests_helper.CHROME_DIR, 'out', 'Release',
          'debug_info_dumps'),
      log_dump_name, [d for d in debug_info_list if d])
  return TestResults.FromTestResults(results)

def Dispatch(options):
  """Dispatches the tests, sharding if possible.

  If options.use_emulator is True, all tests will be run in a new emulator
  instance.

  Args:
    options: options for running the tests.

  Returns:
    0 if successful, number of failing tests otherwise.
  """
  if options.test_suite == 'help':
    ListTestSuites()
    return 0
  buildbot_emulator = None
  attached_devices = []

  if options.use_xvfb:
    xvfb = Xvfb()
    xvfb.Start()

  if options.use_emulator:
    buildbot_emulator = emulator.Emulator()
    buildbot_emulator.Launch()
    attached_devices.append(buildbot_emulator.device)
  else:
    attached_devices = android_commands.GetAttachedDevices()

  if not attached_devices:
    logging.critical('A device must be attached and online.')
    return 1

  test_results = RunTests(attached_devices[0], options.test_suite,
                          options.gtest_filter, options.test_arguments,
                          options.rebaseline, options.timeout,
                          options.performance_test,
                          options.cleanup_test_files, options.tool,
                          options.log_dump)
  if buildbot_emulator:
    buildbot_emulator.Shutdown()
  if options.use_xvfb:
    xvfb.Stop()

  return len(test_results.failed)

def ListTestSuites():
  """Display a list of available test suites
  """
  print 'Available test suites are:'
  for test_suite in _TEST_SUITES:
    print test_suite


def main(argv):
  option_parser = run_tests_helper.CreateTestRunnerOptionParser(None,
      default_timeout=0)
  option_parser.add_option('-s', dest='test_suite',
                           help='Executable name of the test suite to run '
                           '(use -s help to list them)')
  option_parser.add_option('-r', dest='rebaseline',
                           help='Rebaseline and update *testsuite_disabled',
                           action='store_true',
                           default=False)
  option_parser.add_option('-f', dest='gtest_filter',
                           help='gtest filter')
  option_parser.add_option('-a', '--test_arguments', dest='test_arguments',
                           help='Additional arguments to pass to the test')
  option_parser.add_option('-p', dest='performance_test',
                           help='Indicator of performance test',
                           action='store_true',
                           default=False)
  option_parser.add_option('-L', dest='log_dump',
                           help='file name of log dump, which will be put in'
                           'subfolder debug_info_dumps under the same directory'
                           'in where the test_suite exists.')
  option_parser.add_option('-e', '--emulator', dest='use_emulator',
                           help='Run tests in a new instance of emulator',
                           action='store_true',
                           default=False)
  option_parser.add_option('-x', '--xvfb', dest='use_xvfb',
                           action='store_true', default=False,
                           help='Use Xvfb around tests (ignored if not Linux)')
  options, args = option_parser.parse_args(argv)
  if len(args) > 1:
    print 'Unknown argument:', args[1:]
    option_parser.print_usage()
    sys.exit(1)
  run_tests_helper.SetLogLevel(options.verbose_count)
  return Dispatch(options)


if __name__ == '__main__':
  sys.exit(main(sys.argv))
