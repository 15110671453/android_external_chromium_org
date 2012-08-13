# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

import appengine_memcache as memcache
import operator

class BranchUtility(object):
  def __init__(self, base_path, default_branch, fetcher, memcache):
    self._base_path = base_path
    self._default_branch = default_branch
    self._fetcher = fetcher
    self._memcache = memcache

  def GetAllBranchNumbers(self):
    return [self.GetBranchNumberForChannelName(branch)
            for branch in ['dev', 'beta', 'stable', 'trunk', 'local']]

  def SplitChannelNameFromPath(self, path):
    try:
      first, second = path.split('/', 1)
    except ValueError:
      first = path
      second = ''
    if first in ['trunk', 'dev', 'beta', 'stable']:
      return (first, second)
    else:
      return (self._default_branch, path)

  def GetBranchNumberForChannelName(self, channel_name):
    """Returns the branch number for a channel name. If the |channel_name| is
    'trunk' or 'local', then |channel_name| will be returned unchanged. These
    are returned unchanged because 'trunk' has a separate URL from the other
    branches and should be handled differently. 'local' is also a special branch
    for development that should be handled differently.
    """
    if channel_name in ['trunk', 'local']:
      return channel_name

    branch_number = self._memcache.Get(channel_name + '.' + self._base_path,
                                       memcache.MEMCACHE_BRANCH_UTILITY)
    if branch_number is not None:
      return branch_number

    fetch_data = self._fetcher.Fetch(self._base_path).content
    version_json = json.loads(fetch_data)
    branch_numbers = {}
    for entry in version_json:
      if entry['os'] not in ['win', 'linux', 'mac', 'cros']:
        continue
      for version in entry['versions']:
        if version['channel'] != channel_name:
          continue
        if version['true_branch'] not in branch_numbers:
          branch_numbers[version['true_branch']] = 0
        else:
          branch_numbers[version['true_branch']] += 1

    sorted_branches = sorted(branch_numbers.iteritems(),
                             None,
                             operator.itemgetter(1),
                             True)
    # Cache for 24 hours.
    self._memcache.Set(channel_name + '.' + self._base_path,
                       sorted_branches[0][0],
                       memcache.MEMCACHE_BRANCH_UTILITY,
                       time=86400)

    return sorted_branches[0][0]
