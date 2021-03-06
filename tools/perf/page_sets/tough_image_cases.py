# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# pylint: disable=W0401,W0614
from telemetry.page.actions.all_page_actions import *
from telemetry.page import page as page_module
from telemetry.page import page_set as page_set_module


class ToughImageCasesPage(page_module.Page):

  def __init__(self, url, page_set):
    super(ToughImageCasesPage, self).__init__(url=url, page_set=page_set)
    self.user_agent_type = 'desktop'


class ToughImageCasesPageSet(page_set_module.PageSet):

  """ A collection of image-heavy sites. """

  def __init__(self):
    super(ToughImageCasesPageSet, self).__init__(
      user_agent_type='desktop')

    urls_list = [
      'http://www.free-pictures-photos.com/aviation/airplane-306.jpg',
      ('http://upload.wikimedia.org/wikipedia/commons/c/cb/'
       'General_history%2C_Alaska_Yukon_Pacific_Exposition%'
       '2C_fully_illustrated_-_meet_me_in_Seattle_1909_-_Page_78.jpg')
    ]

    for url in urls_list:
      self.AddPage(ToughImageCasesPage(url, self))
