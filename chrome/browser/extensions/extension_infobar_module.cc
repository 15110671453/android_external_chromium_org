// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_infobar_module.h"

#include "app/l10n_util.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/extensions/extension_host.h"
#include "chrome/browser/extensions/extension_infobar_module_constants.h"
#include "chrome/browser/extensions/extension_infobar_delegate.h"
#include "chrome/browser/extensions/extension_tabs_module.h"
#include "chrome/browser/extensions/extension_tabs_module_constants.h"
#include "chrome/browser/tab_contents/infobar_delegate.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/url_constants.h"
#include "grit/generated_resources.h"

namespace keys = extension_infobar_module_constants;

bool ShowInfoBarFunction::RunImpl() {
  DictionaryValue* args;
  EXTENSION_FUNCTION_VALIDATE(args_->GetDictionary(0, &args));

  std::string html_path;
  EXTENSION_FUNCTION_VALIDATE(args->GetString(keys::kHtmlPath, &html_path));

  Extension* extension = GetExtension();
  GURL url = extension->GetResourceURL(extension->url(), html_path);

  Browser* browser = NULL;
  TabContents* tab_contents = NULL;
  if (args->HasKey(keys::kTabId)) {
    int tab_id = -1;
    EXTENSION_FUNCTION_VALIDATE(args->GetInteger(keys::kTabId, &tab_id));

    EXTENSION_FUNCTION_VALIDATE(ExtensionTabUtil::GetTabById(
        tab_id, profile(), true,  // Allow infobar in incognito.
        &browser, NULL, &tab_contents, NULL));
  } else {
    browser = dispatcher()->GetCurrentBrowser(true);
    tab_contents = browser->GetSelectedTabContents();
  }

  if (!browser) {
    error_ = keys::kNoCurrentWindowError;
    return false;
  }

  if (!tab_contents) {
    error_ = keys::kTabNotFoundError;
    return false;
  }

  tab_contents->AddInfoBar(
      new ExtensionInfoBarDelegate(browser, tab_contents, GetExtension(), url));

  // TODO(finnur): Return the actual DOMWindow object. Bug 26463.
  result_.reset(ExtensionTabUtil::CreateWindowValue(browser, false));

  return true;
}
