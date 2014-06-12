# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import re

from telemetry.core import util
from telemetry.page.actions.gesture_action import GestureAction
from telemetry.page.actions import page_action

def _EscapeSelector(selector):
  return selector.replace('\'', '\\\'')


class TapAction(GestureAction):
  def __init__(self, attributes=None):
    super(TapAction, self).__init__(attributes)

  def WillRunAction(self, tab):
    for js_file in ['gesture_common.js', 'tap.js']:
      with open(os.path.join(os.path.dirname(__file__), js_file)) as f:
        js = f.read()
        tab.ExecuteJavaScript(js)

    # Fail if browser doesn't support synthetic tap gestures.
    if not tab.EvaluateJavaScript('window.__TapAction_SupportedByBrowser()'):
      raise page_action.PageActionNotSupported(
          'Synthetic tap not supported for this browser')

    done_callback = 'function() { window.__tapActionDone = true; }'
    tab.ExecuteJavaScript("""
        window.__tapActionDone = false;
        window.__tapAction = new __TapAction(%s);"""
        % (done_callback))

  def HasElementSelector(self):
    return (hasattr(self, 'element_function') or hasattr(self, 'selector') or
            hasattr(self, 'text') or hasattr(self, 'xpath'))

  def TapSelectedElement(self, tab, js_cmd):
    assert self.HasElementSelector()
    if hasattr(self, 'text'):
      callback_code = 'function(element) { %s }' % js_cmd
      util.FindElementAndPerformAction(tab, self.text, callback_code)
    else:
      if hasattr(self, 'element_function'):
        # TODO(chrishenry): This special case is around to not break
        # page sets in other repo. This special case will be deleted when
        # these page sets are updated.
        if self.element_function.strip().startswith('function(callback)'):
          tab.ExecuteJavaScript('(%s)(function(element) { %s });' %
                                (self.element_function, js_cmd))
          return

        element_function = self.element_function
      elif hasattr(self, 'selector'):
        element_function = 'document.querySelector(\'%s\')' % _EscapeSelector(
            self.selector)
      elif hasattr(self, 'xpath'):
        element_function = (
            'document.evaluate("%s", document, null, '
            '    XPathResult.FIRST_ORDERED_NODE_TYPE, null)'
            '    .singleNodeValue') % re.escape(self.xpath)
      else:
        assert False

      tab.ExecuteJavaScript(
          '(function(element) { %s })(%s)' % (js_cmd, element_function))

  def RunGesture(self, tab):
    left_position_percentage = 0.5
    top_position_percentage = 0.5
    duration_ms = 50
    gesture_source_type = GestureAction.GetGestureSourceTypeFromOptions(tab)
    if hasattr(self, 'left_position_percentage'):
      left_position_percentage = self.left_position_percentage
    if hasattr(self, 'top_position_percentage'):
      top_position_percentage = self.top_position_percentage
    if hasattr(self, 'duration_ms'):
      duration_ms = self.duration_ms

    if not self.HasElementSelector():
      tab.ExecuteJavaScript("""
          window.__tapAction.start(
          { element: document.body,
            left_position_percentage: %s,
            top_position_percentage: %s,
            duration_ms: %s,
            gesture_source_type: %s });"""
        % (left_position_percentage,
           top_position_percentage,
           duration_ms,
           gesture_source_type))
    else:
      js_cmd = ("""
          window.__tapAction.start(
             { element: element,
               left_position_percentage: %s,
               top_position_percentage: %s,
               duration_ms: %s,
               gesture_source_type: %s });"""
          % (left_position_percentage,
             top_position_percentage,
             duration_ms,
             gesture_source_type))
      self.TapSelectedElement(tab, js_cmd)

    tab.WaitForJavaScriptExpression('window.__tapActionDone', 60)
