// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/nacl_ui.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/values.h"
#include "chrome/browser/plugins/plugin_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/webplugininfo.h"
#include "grit/browser_resources.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

using base::ASCIIToUTF16;
using base::UserMetricsAction;
using content::BrowserThread;
using content::PluginService;
using content::WebUIMessageHandler;

namespace {

content::WebUIDataSource* CreateNaClUIHTMLSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUINaClHost);

  source->SetUseJsonJSFormatV2();
  source->AddLocalizedString("loadingMessage", IDS_NACL_LOADING_MESSAGE);
  source->AddLocalizedString("naclLongTitle", IDS_NACL_TITLE_MESSAGE);
  source->SetJsonPath("strings.js");
  source->AddResourcePath("about_nacl.css", IDR_ABOUT_NACL_CSS);
  source->AddResourcePath("about_nacl.js", IDR_ABOUT_NACL_JS);
  source->SetDefaultResource(IDR_ABOUT_NACL_HTML);
  return source;
}

////////////////////////////////////////////////////////////////////////////////
//
// NaClDomHandler
//
////////////////////////////////////////////////////////////////////////////////

// The handler for JavaScript messages for the about:flags page.
class NaClDomHandler : public WebUIMessageHandler {
 public:
  NaClDomHandler();
  virtual ~NaClDomHandler();

  // WebUIMessageHandler implementation.
  virtual void RegisterMessages() OVERRIDE;

 private:
  // Callback for the "requestNaClInfo" message.
  void HandleRequestNaClInfo(const base::ListValue* args);

  // Callback for the NaCl plugin information.
  void OnGotPlugins(const std::vector<content::WebPluginInfo>& plugins);

  // A helper callback that receives the result of checking if PNaCl path
  // exists and checking the PNaCl |version|. |is_valid| is true if the PNaCl
  // path that was returned by PathService is valid, and false otherwise.
  void DidCheckPathAndVersion(const std::string* version, bool is_valid);

  // Called when enough information is gathered to return data back to the page.
  void MaybeRespondToPage();

  // Helper for MaybeRespondToPage -- called after enough information
  // is gathered.
  void PopulatePageInformation(base::DictionaryValue* naclInfo);

  // Returns whether the specified plugin is enabled.
  bool isPluginEnabled(size_t plugin_index);

  // Adds information regarding the operating system and chrome version to list.
  void AddOperatingSystemInfo(base::ListValue* list);

  // Adds the list of plugins for NaCl to list.
  void AddPluginList(base::ListValue* list);

  // Adds the information relevant to PNaCl (e.g., enablement, paths, version)
  // to the list.
  void AddPnaclInfo(base::ListValue* list);

  // Adds the information relevant to NaCl to list.
  void AddNaClInfo(base::ListValue* list);

  // Whether the page has requested data.
  bool page_has_requested_data_;

  // Whether the plugin information is ready.
  bool has_plugin_info_;

  // Whether PNaCl path was validated. PathService can return a path
  // that does not exists, so it needs to be validated.
  bool pnacl_path_validated_;
  bool pnacl_path_exists_;
  std::string pnacl_version_string_;

  // Factory for the creating refs in callbacks.
  base::WeakPtrFactory<NaClDomHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(NaClDomHandler);
};

NaClDomHandler::NaClDomHandler()
    : page_has_requested_data_(false),
      has_plugin_info_(false),
      pnacl_path_validated_(false),
      pnacl_path_exists_(false),
      weak_ptr_factory_(this) {
  PluginService::GetInstance()->GetPlugins(base::Bind(
      &NaClDomHandler::OnGotPlugins, weak_ptr_factory_.GetWeakPtr()));
}

NaClDomHandler::~NaClDomHandler() {
}

void NaClDomHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "requestNaClInfo",
      base::Bind(&NaClDomHandler::HandleRequestNaClInfo,
                 base::Unretained(this)));
}

// Helper functions for collecting a list of key-value pairs that will
// be displayed.
void AddPair(base::ListValue* list,
             const base::string16& key,
             const base::string16& value) {
  base::DictionaryValue* results = new base::DictionaryValue();
  results->SetString("key", key);
  results->SetString("value", value);
  list->Append(results);
}

// Generate an empty data-pair which acts as a line break.
void AddLineBreak(base::ListValue* list) {
  AddPair(list, ASCIIToUTF16(""), ASCIIToUTF16(""));
}

bool NaClDomHandler::isPluginEnabled(size_t plugin_index) {
  std::vector<content::WebPluginInfo> info_array;
  PluginService::GetInstance()->GetPluginInfoArray(
      GURL(), "application/x-nacl", false, &info_array, NULL);
  PluginPrefs* plugin_prefs =
      PluginPrefs::GetForProfile(Profile::FromWebUI(web_ui())).get();
  return (!info_array.empty() &&
          plugin_prefs->IsPluginEnabled(info_array[plugin_index]));
}

void NaClDomHandler::AddOperatingSystemInfo(base::ListValue* list) {
  // Obtain the Chrome version info.
  chrome::VersionInfo version_info;
  AddPair(list,
          l10n_util::GetStringUTF16(IDS_PRODUCT_NAME),
          ASCIIToUTF16(version_info.Version() + " (" +
                       chrome::VersionInfo::GetVersionStringModifier() + ")"));

  // OS version information.
  // TODO(jvoung): refactor this to share the extra windows labeling
  // with about:flash, or something.
  std::string os_label = version_info.OSType();
#if defined(OS_WIN)
  base::win::OSInfo* os = base::win::OSInfo::GetInstance();
  switch (os->version()) {
    case base::win::VERSION_XP: os_label += " XP"; break;
    case base::win::VERSION_SERVER_2003:
      os_label += " Server 2003 or XP Pro 64 bit";
      break;
    case base::win::VERSION_VISTA: os_label += " Vista or Server 2008"; break;
    case base::win::VERSION_WIN7: os_label += " 7 or Server 2008 R2"; break;
    case base::win::VERSION_WIN8: os_label += " 8 or Server 2012"; break;
    default:  os_label += " UNKNOWN"; break;
  }
  os_label += " SP" + base::IntToString(os->service_pack().major);
  if (os->service_pack().minor > 0)
    os_label += "." + base::IntToString(os->service_pack().minor);
  if (os->architecture() == base::win::OSInfo::X64_ARCHITECTURE)
    os_label += " 64 bit";
#endif
  AddPair(list,
          l10n_util::GetStringUTF16(IDS_ABOUT_VERSION_OS),
          ASCIIToUTF16(os_label));
  AddLineBreak(list);
}

void NaClDomHandler::AddPluginList(base::ListValue* list) {
  // Obtain the version of the NaCl plugin.
  std::vector<content::WebPluginInfo> info_array;
  PluginService::GetInstance()->GetPluginInfoArray(
      GURL(), "application/x-nacl", false, &info_array, NULL);
  base::string16 nacl_version;
  base::string16 nacl_key = ASCIIToUTF16("NaCl plugin");
  if (info_array.empty()) {
    AddPair(list, nacl_key, ASCIIToUTF16("Disabled"));
  } else {
    // Only the 0th plugin is used.
    nacl_version = info_array[0].version + ASCIIToUTF16(" ") +
        info_array[0].path.LossyDisplayName();
    if (!isPluginEnabled(0)) {
      nacl_version += ASCIIToUTF16(" (Disabled in profile prefs)");
    }

    AddPair(list, nacl_key, nacl_version);

    // Mark the rest as not used.
    for (size_t i = 1; i < info_array.size(); ++i) {
      nacl_version = info_array[i].version + ASCIIToUTF16(" ") +
          info_array[i].path.LossyDisplayName();
      nacl_version += ASCIIToUTF16(" (not used)");
      if (!isPluginEnabled(i)) {
        nacl_version += ASCIIToUTF16(" (Disabled in profile prefs)");
      }
      AddPair(list, nacl_key, nacl_version);
    }
  }
  AddLineBreak(list);
}

void NaClDomHandler::AddPnaclInfo(base::ListValue* list) {
  // Display whether PNaCl is enabled.
  base::string16 pnacl_enabled_string = ASCIIToUTF16("Enabled");
  if (!isPluginEnabled(0)) {
    pnacl_enabled_string = ASCIIToUTF16("Disabled in profile prefs");
  } else if (CommandLine::ForCurrentProcess()->HasSwitch(
                 switches::kDisablePnacl)) {
    pnacl_enabled_string = ASCIIToUTF16("Disabled by flag '--disable-pnacl'");
  }
  AddPair(list,
          ASCIIToUTF16("Portable Native Client (PNaCl)"),
          pnacl_enabled_string);

  // Obtain the version of the PNaCl translator.
  base::FilePath pnacl_path;
  bool got_path = PathService::Get(chrome::DIR_PNACL_COMPONENT, &pnacl_path);
  if (!got_path || pnacl_path.empty() || !pnacl_path_exists_) {
    AddPair(list,
            ASCIIToUTF16("PNaCl translator"),
            ASCIIToUTF16("Not installed"));
  } else {
    AddPair(list,
            ASCIIToUTF16("PNaCl translator path"),
            pnacl_path.LossyDisplayName());
    AddPair(list,
            ASCIIToUTF16("PNaCl translator version"),
            ASCIIToUTF16(pnacl_version_string_));
  }
  AddLineBreak(list);
}

void NaClDomHandler::AddNaClInfo(base::ListValue* list) {
  base::string16 nacl_enabled_string = ASCIIToUTF16("Disabled");
  if (isPluginEnabled(0) &&
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableNaCl)) {
    nacl_enabled_string = ASCIIToUTF16("Enabled by flag '--enable-nacl'");
  }
  AddPair(list,
          ASCIIToUTF16("Native Client (non-portable, outside web store)"),
          nacl_enabled_string);
  AddLineBreak(list);
}

void NaClDomHandler::HandleRequestNaClInfo(const base::ListValue* args) {
  page_has_requested_data_ = true;
  // Force re-validation of PNaCl's path in the next call to
  // MaybeRespondToPage(), in case PNaCl went from not-installed
  // to installed since the request.
  pnacl_path_validated_ = false;
  MaybeRespondToPage();
}

void NaClDomHandler::OnGotPlugins(
    const std::vector<content::WebPluginInfo>& plugins) {
  has_plugin_info_ = true;
  MaybeRespondToPage();
}

void NaClDomHandler::PopulatePageInformation(base::DictionaryValue* naclInfo) {
  DCHECK(pnacl_path_validated_);
  // Store Key-Value pairs of about-information.
  scoped_ptr<base::ListValue> list(new base::ListValue());
  // Display the operating system and chrome version information.
  AddOperatingSystemInfo(list.get());
  // Display the list of plugins serving NaCl.
  AddPluginList(list.get());
  // Display information relevant to PNaCl.
  AddPnaclInfo(list.get());
  // Display information relevant to NaCl (non-portable.
  AddNaClInfo(list.get());
  // naclInfo will take ownership of list, and clean it up on destruction.
  naclInfo->Set("naclInfo", list.release());
}

void NaClDomHandler::DidCheckPathAndVersion(const std::string* version,
                                            bool is_valid) {
  pnacl_path_validated_ = true;
  pnacl_path_exists_ = is_valid;
  pnacl_version_string_ = *version;
  MaybeRespondToPage();
}

void CheckVersion(const base::FilePath& pnacl_path, std::string* version) {
  base::FilePath pnacl_json_path =
      pnacl_path.AppendASCII("pnacl_public_pnacl_json");
  JSONFileValueSerializer serializer(pnacl_json_path);
  std::string error;
  scoped_ptr<base::Value> root(serializer.Deserialize(NULL, &error));
  if (!root || !root->IsType(base::Value::TYPE_DICTIONARY))
    return;

  // Now try to get the field. This may leave version empty if the
  // the "get" fails (no key, or wrong type).
  static_cast<base::DictionaryValue*>(root.get())->GetStringASCII(
      "pnacl-version", version);
}

bool CheckPathAndVersion(std::string* version) {
  base::FilePath pnacl_path;
  bool got_path = PathService::Get(chrome::DIR_PNACL_COMPONENT, &pnacl_path);
  if (got_path && !pnacl_path.empty() && base::PathExists(pnacl_path)) {
    CheckVersion(pnacl_path, version);
    return true;
  }
  return false;
}

void NaClDomHandler::MaybeRespondToPage() {
  // Don't reply until everything is ready.  The page will show a 'loading'
  // message until then.
  if (!page_has_requested_data_ || !has_plugin_info_)
    return;

  if (!pnacl_path_validated_) {
    std::string* version_string = new std::string;
    base::PostTaskAndReplyWithResult(
        BrowserThread::GetBlockingPool(),
        FROM_HERE,
        base::Bind(&CheckPathAndVersion, version_string),
        base::Bind(&NaClDomHandler::DidCheckPathAndVersion,
                   weak_ptr_factory_.GetWeakPtr(),
                   base::Owned(version_string)));
    return;
  }

  base::DictionaryValue naclInfo;
  PopulatePageInformation(&naclInfo);
  web_ui()->CallJavascriptFunction("nacl.returnNaClInfo", naclInfo);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// NaClUI
//
///////////////////////////////////////////////////////////////////////////////

NaClUI::NaClUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  content::RecordAction(UserMetricsAction("ViewAboutNaCl"));

  web_ui->AddMessageHandler(new NaClDomHandler());

  // Set up the about:nacl source.
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, CreateNaClUIHTMLSource());
}
