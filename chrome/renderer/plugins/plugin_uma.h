// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PLUGINS_PLUGIN_UMA_H_
#define CHROME_RENDERER_PLUGINS_PLUGIN_UMA_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "googleurl/src/gurl.h"

// Used to send UMA data about missing plugins to UMA histogram server. Method
// ReportPluginMissing should be called whenever plugin that is not available or
// enabled is called. We try to determine plugin's type by requested mime type,
// or, if mime type is unknown, by plugin's src url.
class PluginUMAReporter {
 public:
  enum ReportType {
    MISSING_PLUGIN,
    DISABLED_PLUGIN
  };

  // This must be sync'd with histogram values.
  enum PluginType {
    WINDOWS_MEDIA_PLAYER = 0,
    SILVERLIGHT = 1,
    REALPLAYER = 2,
    JAVA = 3,
    QUICKTIME = 4,
    OTHER = 5
  };

  // Sends UMA data, i.e. plugin's type.
  class UMASender {
   public:
    virtual ~UMASender() {}
    virtual void SendPluginUMA(ReportType report_type,
                               PluginType plugin_type) = 0;
  };

  // Returns singleton instance.
  static PluginUMAReporter* GetInstance();

  void ReportPluginMissing(const std::string& plugin_mime_type,
                           const GURL& plugin_src);

  void ReportPluginDisabled(const std::string& plugin_mime_type,
                            const GURL& plugin_src);

 private:
  friend struct DefaultSingletonTraits<PluginUMAReporter>;
  friend class PluginUMATest;

  PluginUMAReporter();
  ~PluginUMAReporter();

  static bool CompareCStrings(const char* first, const char* second);
  bool CStringArrayContainsCString(const char** array,
                                   size_t array_size,
                                   const char* str);
  // Extracts file extension from url.
  void ExtractFileExtension(const GURL& src, std::string* extension);

  PluginType GetPluginType(const std::string& plugin_mime_type,
                           const GURL& plugin_src);

  // Converts plugin's src to plugin type.
  PluginType SrcToPluginType(const GURL& src);
  // Converts plugin's mime type to plugin type.
  PluginType MimeTypeToPluginType(const std::string& mime_type);

  scoped_ptr<UMASender> report_sender_;

  DISALLOW_COPY_AND_ASSIGN(PluginUMAReporter);
};

#endif  // CHROME_RENDERER_PLUGINS_PLUGIN_UMA_H_

