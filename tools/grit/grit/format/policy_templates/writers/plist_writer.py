# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from xml.dom import minidom
from grit.format.policy_templates.writers import plist_helper
from grit.format.policy_templates.writers import xml_formatted_writer


def GetWriter(config):
  '''Factory method for creating PListWriter objects.
  See the constructor of TemplateWriter for description of
  arguments.
  '''
  return PListWriter(['mac'], config)


class PListWriter(xml_formatted_writer.XMLFormattedWriter):
  '''Class for generating policy templates in Mac plist format.
  It is used by PolicyTemplateGenerator to write plist files.
  '''

  STRING_TABLE = 'Localizable.strings'
  TYPE_TO_INPUT = {
    'string': 'string',
    'int': 'integer',
    'int-enum': 'integer',
    'string-enum': 'string',
    'main': 'boolean',
    'list': 'array',
  }

  def _AddKeyValuePair(self, parent, key_string, value_tag):
    '''Adds a plist key-value pair to a parent XML element.

    A key-value pair in plist consists of two XML elements next two each other:
    <key>key_string</key>
    <value_tag>...</value_tag>

    Args:
      key_string: The content of the key tag.
      value_tag: The name of the value element.

    Returns:
      The XML element of the value tag.
    '''
    self.AddElement(parent, 'key', {}, key_string)
    return self.AddElement(parent, value_tag)

  def _AddStringKeyValuePair(self, parent, key_string, value_string):
    '''Adds a plist key-value pair to a parent XML element, where the
    value element contains a string. The name of the value element will be
    <string>.

    Args:
      key_string: The content of the key tag.
      value_string: The content of the value tag.
    '''
    self.AddElement(parent, 'key', {}, key_string)
    self.AddElement(parent, 'string', {}, value_string)

  def _AddTargets(self, parent):
    '''Adds the following XML snippet to an XML element:
      <key>pfm_targets</key>
      <array>
        <string>user-managed</string>
      </array>

      Args:
        parent: The parent XML element where the snippet will be added.
    '''
    array = self._AddKeyValuePair(parent, 'pfm_targets', 'array')
    self.AddElement(array, 'string', {}, 'user-managed')

  def PreprocessPolicies(self, policy_list):
    return self.FlattenGroupsAndSortPolicies(policy_list)

  def WritePolicy(self, policy):
    policy_name = policy['name']
    policy_type = policy['type']

    dict = self.AddElement(self._array, 'dict')
    self._AddStringKeyValuePair(dict, 'pfm_name', policy_name)
    # Set empty strings for title and description. They will be taken by the
    # OSX Workgroup Manager from the string table in a Localizable.strings file.
    # Those files are generated by plist_strings_writer.
    self._AddStringKeyValuePair(dict, 'pfm_description', '')
    self._AddStringKeyValuePair(dict, 'pfm_title', '')
    self._AddTargets(dict)
    self._AddStringKeyValuePair(dict, 'pfm_type',
                                self.TYPE_TO_INPUT[policy_type])
    if policy_type in ('int-enum', 'string-enum'):
      range_list = self._AddKeyValuePair(dict, 'pfm_range_list', 'array')
      for item in policy['items']:
        if policy_type == 'int-enum':
          element_type = 'integer'
        else:
          element_type = 'string'
        self.AddElement(range_list, element_type, {}, str(item['value']))

  def BeginTemplate(self):
    self._plist.attributes['version'] = '1'
    dict = self.AddElement(self._plist, 'dict')

    app_name = plist_helper.GetPlistFriendlyName(self.config['app_name'])
    self._AddStringKeyValuePair(dict, 'pfm_name', app_name)
    self._AddStringKeyValuePair(dict, 'pfm_description', '')
    self._AddStringKeyValuePair(dict, 'pfm_title', '')
    self._AddStringKeyValuePair(dict, 'pfm_version', '1')
    self._AddStringKeyValuePair(dict, 'pfm_domain',
                                self.config['mac_bundle_id'])

    self._array = self._AddKeyValuePair(dict, 'pfm_subkeys', 'array')

  def Init(self):
    dom_impl = minidom.getDOMImplementation('')
    doctype = dom_impl.createDocumentType(
        'plist',
        '-//Apple//DTD PLIST 1.0//EN',
        'http://www.apple.com/DTDs/PropertyList-1.0.dtd')
    self._doc = dom_impl.createDocument(None, 'plist', doctype)
    self._plist = self._doc.documentElement

  def GetTemplateText(self):
    # return self.plist_doc.toprettyxml(indent='  ')
    # The above pretty-printer does not print the doctype and adds spaces
    # around texts, which the OSX Workgroup Manager does not like. So we use
    # the poor man's pretty printer. It assumes that there are no mixed-content
    # nodes.
    # Get all the XML content in a one-line string.
    xml = self._doc.toxml()
    # Determine where the line breaks will be. (They will only be between tags.)
    lines = xml[1:len(xml) - 1].split('><')
    indent = ''
    res = ''
    # Determine indent for each line.
    for i in range(len(lines)):
      line = lines[i]
      if line[0] == '/':
        # If the current line starts with a closing tag, decrease indent before
        # printing.
        indent = indent[2:]
      lines[i] = indent + '<' + line + '>'
      if (line[0] not in ['/', '?', '!'] and '</' not in line and
          line[len(line) - 1] != '/'):
        # If the current line starts with an opening tag and does not conatin a
        # closing tag, increase indent after the line is printed.
        indent += '  '
    # Reconstruct XML text from the lines.
    return '\n'.join(lines)
