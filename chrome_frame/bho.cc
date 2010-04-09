// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_frame/bho.h"

#include <shlguid.h>

#include "base/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/registry.h"
#include "base/scoped_bstr_win.h"
#include "base/scoped_variant_win.h"
#include "base/string_util.h"
#include "chrome_tab.h" // NOLINT
#include "chrome_frame/extra_system_apis.h"
#include "chrome_frame/http_negotiate.h"
#include "chrome_frame/protocol_sink_wrap.h"
#include "chrome_frame/urlmon_moniker.h"
#include "chrome_frame/utils.h"
#include "chrome_frame/vtable_patch_manager.h"

static const int kIBrowserServiceOnHttpEquivIndex = 30;

PatchHelper g_patch_helper;

BEGIN_VTABLE_PATCHES(IBrowserService)
  VTABLE_PATCH_ENTRY(kIBrowserServiceOnHttpEquivIndex, Bho::OnHttpEquiv)
END_VTABLE_PATCHES()

_ATL_FUNC_INFO Bho::kBeforeNavigate2Info = {
  CC_STDCALL, VT_EMPTY, 7, {
    VT_DISPATCH,
    VT_VARIANT | VT_BYREF,
    VT_VARIANT | VT_BYREF,
    VT_VARIANT | VT_BYREF,
    VT_VARIANT | VT_BYREF,
    VT_VARIANT | VT_BYREF,
    VT_BOOL | VT_BYREF
  }
};

_ATL_FUNC_INFO Bho::kNavigateComplete2Info = {
  CC_STDCALL, VT_EMPTY, 2, {
    VT_DISPATCH,
    VT_VARIANT | VT_BYREF
  }
};

Bho::Bho() {
}

HRESULT Bho::FinalConstruct() {
  return S_OK;
}

void Bho::FinalRelease() {
}

STDMETHODIMP Bho::SetSite(IUnknown* site) {
  HRESULT hr = S_OK;
  if (site) {
    ScopedComPtr<IWebBrowser2> web_browser2;
    web_browser2.QueryFrom(site);
    if (web_browser2) {
      hr = DispEventAdvise(web_browser2, &DIID_DWebBrowserEvents2);
      DCHECK(SUCCEEDED(hr)) << "DispEventAdvise failed. Error: " << hr;
    }

    if (g_patch_helper.state() == PatchHelper::PATCH_IBROWSER) {
      ScopedComPtr<IBrowserService> browser_service;
      hr = DoQueryService(SID_SShellBrowser, site, browser_service.Receive());
      DCHECK(browser_service) << "DoQueryService - SID_SShellBrowser failed."
          << " Site: " << site << " Error: " << hr;
      if (browser_service) {
        g_patch_helper.PatchBrowserService(browser_service);
        DCHECK(SUCCEEDED(hr)) << "vtable_patch::PatchInterfaceMethods failed."
            << " Site: " << site << " Error: " << hr;
      }
    }
    // Save away our BHO instance in TLS which enables it to be referenced by
    // our active document/activex instances to query referrer and other
    // information for a URL.
    AddRef();
    RegisterThreadInstance();
  } else {
    UnregisterThreadInstance();
    Release();
  }

  return IObjectWithSiteImpl<Bho>::SetSite(site);
}

STDMETHODIMP Bho::BeforeNavigate2(IDispatch* dispatch, VARIANT* url,
    VARIANT* flags, VARIANT* target_frame_name, VARIANT* post_data,
    VARIANT* headers, VARIANT_BOOL* cancel) {
  if (!url || url->vt != VT_BSTR || url->bstrVal == NULL) {
    DLOG(WARNING) << "Invalid URL passed in";
    return S_OK;
  }

  ScopedComPtr<IWebBrowser2> web_browser2;
  if (dispatch)
    web_browser2.QueryFrom(dispatch);

  if (!web_browser2) {
    NOTREACHED() << "Can't find WebBrowser2 with given dispatch";
    return S_OK;
  }

  DLOG(INFO) << "BeforeNavigate2: " << url->bstrVal;
  ScopedComPtr<IBrowserService> browser_service;
  DoQueryService(SID_SShellBrowser, web_browser2, browser_service.Receive());
  if (!browser_service || !CheckForCFNavigation(browser_service, false)) {
    referrer_.clear();
  }

  VARIANT_BOOL is_top_level = VARIANT_FALSE;
  web_browser2->get_TopLevelContainer(&is_top_level);
  if (is_top_level) {
    set_url(url->bstrVal);
    ProcessOptInUrls(web_browser2, url->bstrVal);
  }

  return S_OK;
}

STDMETHODIMP_(void) Bho::NavigateComplete2(IDispatch* dispatch, VARIANT* url) {
  DLOG(INFO) << __FUNCTION__;
}

namespace {

// See comments in Bho::OnHttpEquiv for details.
void ClearDocumentContents(IUnknown* browser) {
  ScopedComPtr<IWebBrowser2> web_browser2;
  if (SUCCEEDED(DoQueryService(SID_SWebBrowserApp, browser,
                               web_browser2.Receive()))) {
    ScopedComPtr<IDispatch> doc_disp;
    web_browser2->get_Document(doc_disp.Receive());
    ScopedComPtr<IHTMLDocument2> doc;
    if (doc_disp && SUCCEEDED(doc.QueryFrom(doc_disp))) {
      SAFEARRAY* sa = ::SafeArrayCreateVector(VT_UI1, 0, 0);
      doc->write(sa);
      ::SafeArrayDestroy(sa);
    }
  }
}

// Returns true if the currently loaded document in the browser has
// any embedded items such as a frame or an iframe.
bool DocumentHasEmbeddedItems(IUnknown* browser) {
  bool has_embedded_items = false;

  ScopedComPtr<IWebBrowser2> web_browser2;
  ScopedComPtr<IDispatch> document;
  if (SUCCEEDED(DoQueryService(SID_SWebBrowserApp, browser,
                               web_browser2.Receive())) &&
      SUCCEEDED(web_browser2->get_Document(document.Receive()))) {
    ScopedComPtr<IOleContainer> container;
    if (SUCCEEDED(container.QueryFrom(document))) {
      ScopedComPtr<IEnumUnknown> enumerator;
      container->EnumObjects(OLECONTF_EMBEDDINGS, enumerator.Receive());
      if (enumerator) {
        ScopedComPtr<IUnknown> unk;
        DWORD fetched = 0;
        while (!has_embedded_items &&
               SUCCEEDED(enumerator->Next(1, unk.Receive(), &fetched))
               && fetched) {
          // If a top level document has embedded iframes then the theory is
          // that first the top level document finishes loading and then the
          // iframes load. We should only treat an embedded element as an
          // iframe if it supports the IWebBrowser interface.
          ScopedComPtr<IWebBrowser2> embedded_web_browser2;
          if (SUCCEEDED(embedded_web_browser2.QueryFrom(unk))) {
            // If we initiate a top level navigation then at times MSHTML
            // creates a temporary IWebBrowser2 interface which basically shows
            // up as a temporary iframe in the parent document. It is not clear
            // as to how we can detect this. I tried the usual stuff like
            // getting to the parent IHTMLWindow2 interface. They all end up
            // pointing to dummy tear off interfaces owned by MSHTML.
            // As a temporary workaround, we found that the location url in
            // this case is about:blank. We now check for the same and don't
            // treat it as an iframe. This should be fine in most cases as we
            // hit this code only when the actual page has a meta tag. However
            // this would break for cases like the initial src url for an
            // iframe pointing to about:blank and the page then writing to it
            // via document.write.
            // TODO(ananta)
            // Revisit this and come up with a better approach.
            ScopedBstr location_url;
            embedded_web_browser2->get_LocationURL(location_url.Receive());

            std::wstring location_url_string;
            location_url_string.assign(location_url, location_url.Length());

            if (!LowerCaseEqualsASCII(location_url_string, "about:blank")) {
              has_embedded_items = true;
            }
          }

          fetched = 0;
          unk.Release();
        }
      }
    }
  }

  return has_embedded_items;
}

}  // end namespace

HRESULT Bho::OnHttpEquiv(IBrowserService_OnHttpEquiv_Fn original_httpequiv,
    IBrowserService* browser, IShellView* shell_view, BOOL done,
    VARIANT* in_arg, VARIANT* out_arg) {
  DLOG(INFO) << __FUNCTION__ << " done:" << done;

  // OnHttpEquiv with 'done' set to TRUE is called for all pages.
  // 0 or more calls with done set to FALSE are made.
  // When done is FALSE, the current moniker may not represent the page
  // being navigated to so we always have to wait for done to be TRUE
  // before re-initiating the navigation.

  if (!done && in_arg && VT_BSTR == V_VT(in_arg)) {
    if (StrStrI(V_BSTR(in_arg), kChromeContentPrefix)) {
      // OnHttpEquiv is invoked for meta tags within sub frames as well.
      // We want to switch renderers only for the top level frame.
      // The theory here is that if there are any existing embedded items
      // (frames or iframes) in the current document, then the http-equiv
      // notification is coming from those and not the top level document.
      // The embedded items should only be created once the top level
      // doc has been created.
      if (!DocumentHasEmbeddedItems(browser)) {
        NavigationManager* mgr = NavigationManager::GetThreadInstance();
        DCHECK(mgr);
        DLOG(INFO) << "Found tag in page. Marking browser." <<
            StringPrintf(" tid=0x%08X", ::GetCurrentThreadId());
        if (mgr) {
          // TODO(tommi): See if we can't figure out a cleaner way to avoid
          // this.  For some documents we can hit a problem here.  When we
          // attempt to navigate the document again in CF, mshtml can "complete"
          // the current navigation (if all data is available) and fire off
          // script events such as onload and even render the page.
          // This will happen inside NavigateBrowserToMoniker below.
          // To work around this, we clear the contents of the document before
          // opening it up in CF.
          ClearDocumentContents(browser);
          mgr->NavigateToCurrentUrlInCF(browser);
        }
      }
    }
  }

  return original_httpequiv(browser, shell_view, done, in_arg, out_arg);
}

// static
void Bho::ProcessOptInUrls(IWebBrowser2* browser, BSTR url) {
  if (!browser || !url) {
    NOTREACHED();
    return;
  }

#ifndef NDEBUG
  // This check must already have been made.
  VARIANT_BOOL is_top_level = VARIANT_FALSE;
  browser->get_TopLevelContainer(&is_top_level);
  DCHECK(is_top_level);
#endif

  std::wstring current_url(url, SysStringLen(url));
  if (IsValidUrlScheme(current_url, false)) {
    bool cf_protocol = StartsWith(current_url, kChromeProtocolPrefix, false);
    if (!cf_protocol && IsOptInUrl(current_url.c_str())) {
      DLOG(INFO) << "Opt-in URL. Switching to cf.";
      ScopedComPtr<IBrowserService> browser_service;
      DoQueryService(SID_SShellBrowser, browser, browser_service.Receive());
      DCHECK(browser_service) << "DoQueryService - SID_SShellBrowser failed.";
      MarkBrowserOnThreadForCFNavigation(browser_service);
    }
  }
}

namespace {
// Utility function that prevents the current module from ever being unloaded.
void PinModule() {
  FilePath module_path;
  if (PathService::Get(base::FILE_MODULE, &module_path)) {
    HMODULE unused;
    if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_PIN,
                           module_path.value().c_str(), &unused)) {
      NOTREACHED() << "Failed to pin module " << module_path.value().c_str()
                   << " , last error: " << GetLastError();
    }
  } else {
    NOTREACHED() << "Could not get module path.";
  }
}
}  // namespace

bool PatchHelper::InitializeAndPatchProtocolsIfNeeded() {
  bool ret = false;

  _pAtlModule->m_csStaticDataInitAndTypeInfo.Lock();

  if (state_ == UNKNOWN) {
    // If we're going to start patching things for reals, we'd better make sure
    // that we stick around for ever more:
    if (!IsUnpinnedMode())
      PinModule();

    HttpNegotiatePatch::Initialize();

    ProtocolPatchMethod patch_method =
        static_cast<ProtocolPatchMethod>(
            GetConfigInt(PATCH_METHOD_IBROWSER, kPatchProtocols));
    if (patch_method == PATCH_METHOD_INET_PROTOCOL) {
      ProtocolSinkWrap::PatchProtocolHandlers();
      state_ = PATCH_PROTOCOL;
    } else if (patch_method == PATCH_METHOD_IBROWSER) {
        state_ =  PATCH_IBROWSER;
    } else {
      DCHECK(patch_method == PATCH_METHOD_MONIKER);
      state_ = PATCH_MONIKER;
      MonikerPatch::Initialize();
    }

    ret = true;
  }

  _pAtlModule->m_csStaticDataInitAndTypeInfo.Unlock();

  return ret;
}

void PatchHelper::PatchBrowserService(IBrowserService* browser_service) {
  DCHECK(state_ == PATCH_IBROWSER);
  if (!IS_PATCHED(IBrowserService)) {
    vtable_patch::PatchInterfaceMethods(browser_service,
                                        IBrowserService_PatchInfo);
  }
}

void PatchHelper::UnpatchIfNeeded() {
  if (state_ == PATCH_PROTOCOL) {
    ProtocolSinkWrap::UnpatchProtocolHandlers();
  } else if (state_ == PATCH_IBROWSER) {
    vtable_patch::UnpatchInterfaceMethods(IBrowserService_PatchInfo);
    MonikerPatch::Uninitialize();
  }

  HttpNegotiatePatch::Uninitialize();

  state_ = UNKNOWN;
}

