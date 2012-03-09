// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_CONFIGURATION_POLICY_HANDLER_H_
#define CHROME_BROWSER_POLICY_CONFIGURATION_POLICY_HANDLER_H_
#pragma once

#include <vector>

#include "base/basictypes.h"
#include "base/values.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"

class PrefValueMap;

namespace policy {

class PolicyErrorMap;
class PolicyMap;

// An abstract super class that subclasses should implement to map policies to
// their corresponding preferences, and to check whether the policies are valid.
class ConfigurationPolicyHandler {
 public:
  ConfigurationPolicyHandler();
  virtual ~ConfigurationPolicyHandler();

  // Returns true if the policy settings handled by this
  // ConfigurationPolicyHandler can be applied and false otherwise. Fills
  // |errors| with error messages or warnings. |errors| may contain error
  // messages even when |CheckPolicySettings()| returns true.
  virtual bool CheckPolicySettings(const PolicyMap& policies,
                                   PolicyErrorMap* errors) = 0;

  // Processes the policies handled by this ConfigurationPolicyHandler and sets
  // the appropriate preferences in |prefs|.
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) = 0;

  // Modifies the values of some of the policies in |policies| so that they
  // are more suitable to display to the user. This can be used to remove
  // sensitive values such as passwords, or to pretty-print values.
  // The base implementation just converts DictionaryValue policies to a
  // StringValue representation.
  virtual void PrepareForDisplaying(PolicyMap* policies) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(ConfigurationPolicyHandler);
};

// Abstract class derived from ConfigurationPolicyHandler that should be
// subclassed to handle a single policy (not a combination of policies).
class TypeCheckingPolicyHandler : public ConfigurationPolicyHandler {
 public:
  TypeCheckingPolicyHandler(const char* policy_name,
                            base::Value::Type value_type);

  // ConfigurationPolicyHandler methods:
  virtual bool CheckPolicySettings(const PolicyMap& policies,
                                   PolicyErrorMap* errors) OVERRIDE;

 protected:
  virtual ~TypeCheckingPolicyHandler();

  // Runs policy checks and returns the policy value if successful.
  bool CheckAndGetValue(const PolicyMap& policies,
                        PolicyErrorMap* errors,
                        const Value** value);

  const char* policy_name() const;

 private:
  // The name of the policy.
  const char* policy_name_;

  // The type the value of the policy should have.
  base::Value::Type value_type_;

  DISALLOW_COPY_AND_ASSIGN(TypeCheckingPolicyHandler);
};

// ConfigurationPolicyHandler for policies that map directly to a preference.
class SimplePolicyHandler : public TypeCheckingPolicyHandler {
 public:
  SimplePolicyHandler(const char* policy_name,
                      const char* pref_path,
                      base::Value::Type value_type);
  virtual ~SimplePolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  // The DictionaryValue path of the preference the policy maps to.
  const char* pref_path_;

  DISALLOW_COPY_AND_ASSIGN(SimplePolicyHandler);
};

// ConfigurationPolicyHandler for the SyncDisabled policy.
class SyncPolicyHandler : public TypeCheckingPolicyHandler {
 public:
  SyncPolicyHandler();
  virtual ~SyncPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(SyncPolicyHandler);
};

// ConfigurationPolicyHandler for the AutofillEnabled policy.
class AutofillPolicyHandler : public TypeCheckingPolicyHandler {
 public:
  AutofillPolicyHandler();
  virtual ~AutofillPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(AutofillPolicyHandler);
};

// ConfigurationPolicyHandler for the DownloadDirectory policy.
class DownloadDirPolicyHandler : public TypeCheckingPolicyHandler {
 public:
  DownloadDirPolicyHandler();
  virtual ~DownloadDirPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadDirPolicyHandler);
};

// ConfigurationPolicyHandler for the DiskCacheDir policy.
class DiskCacheDirPolicyHandler : public TypeCheckingPolicyHandler {
 public:
  explicit DiskCacheDirPolicyHandler();
  virtual ~DiskCacheDirPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(DiskCacheDirPolicyHandler);
};

// ConfigurationPolicyHandler for the FileSelectionDialogsHandler policy.
class FileSelectionDialogsHandler : public TypeCheckingPolicyHandler {
 public:
  FileSelectionDialogsHandler();
  virtual ~FileSelectionDialogsHandler();

  // ConfigurationPolicyHandler methods:
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileSelectionDialogsHandler);
};

// ConfigurationPolicyHandler for the incognito mode policies.
class IncognitoModePolicyHandler : public ConfigurationPolicyHandler {
 public:
  IncognitoModePolicyHandler();
  virtual ~IncognitoModePolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual bool CheckPolicySettings(const PolicyMap& policies,
                                   PolicyErrorMap* errors) OVERRIDE;
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  IncognitoModePrefs::Availability GetAvailabilityValueAsEnum(
      const Value* availability);

  DISALLOW_COPY_AND_ASSIGN(IncognitoModePolicyHandler);
};

// ConfigurationPolicyHandler for the DefaultSearchEncodings policy.
class DefaultSearchEncodingsPolicyHandler : public TypeCheckingPolicyHandler {
 public:
  DefaultSearchEncodingsPolicyHandler();
  virtual ~DefaultSearchEncodingsPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultSearchEncodingsPolicyHandler);
};

// ConfigurationPolicyHandler for the default search policies.
class DefaultSearchPolicyHandler : public ConfigurationPolicyHandler {
 public:
  DefaultSearchPolicyHandler();
  virtual ~DefaultSearchPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual bool CheckPolicySettings(const PolicyMap& policies,
                                   PolicyErrorMap* errors) OVERRIDE;
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  // Calls |CheckPolicySettings()| on each of the handlers in |handlers_|
  // and returns true if all of the calls return true and false otherwise.
  bool CheckIndividualPolicies(const PolicyMap& policies,
                               PolicyErrorMap* errors);

  // Returns true if there is a value for |policy_name| in |policies| and false
  // otherwise.
  bool HasDefaultSearchPolicy(const PolicyMap& policies,
                              const char* policy_name);

  // Returns true if any default search policies are specified in |policies| and
  // false otherwise.
  bool AnyDefaultSearchPoliciesSpecified(const PolicyMap& policies);

  // Returns true if the default search provider is disabled and false
  // otherwise.
  bool DefaultSearchProviderIsDisabled(const PolicyMap& policies);

  // Returns true if the default search URL was set and is valid and false
  // otherwise.
  bool DefaultSearchURLIsValid(const PolicyMap& policies);

  // Make sure that the |path| if present in |prefs_|.  If not, set it to
  // a blank string.
  void EnsureStringPrefExists(PrefValueMap* prefs, const std::string& path);

  // The ConfigurationPolicyHandler handlers for each default search policy.
  std::vector<ConfigurationPolicyHandler*> handlers_;

  DISALLOW_COPY_AND_ASSIGN(DefaultSearchPolicyHandler);
};

// ConfigurationPolicyHandler for the proxy policies.
class ProxyPolicyHandler : public ConfigurationPolicyHandler {
 public:
  // Constants for the "Proxy Server Mode" defined in the policies.
  // Note that these diverge from internal presentation defined in
  // ProxyPrefs::ProxyMode for legacy reasons. The following four
  // PolicyProxyModeType types were not very precise and had overlapping use
  // cases.
  enum ProxyModeType {
    // Disable Proxy, connect directly.
    PROXY_SERVER_MODE = 0,
    // Auto detect proxy or use specific PAC script if given.
    PROXY_AUTO_DETECT_PROXY_SERVER_MODE = 1,
    // Use manually configured proxy servers (fixed servers).
    PROXY_MANUALLY_CONFIGURED_PROXY_SERVER_MODE = 2,
    // Use system proxy server.
    PROXY_USE_SYSTEM_PROXY_SERVER_MODE = 3,

    MODE_COUNT
  };

  ProxyPolicyHandler();
  virtual ~ProxyPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual bool CheckPolicySettings(const PolicyMap& policies,
                                   PolicyErrorMap* errors) OVERRIDE;
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  const Value* GetProxyPolicyValue(const PolicyMap& policies,
                                   const char* policy_name);

  // Converts the deprecated ProxyServerMode policy value to a ProxyMode value
  // and places the result in |mode_value|. Returns true if the conversion
  // succeeded and false otherwise.
  bool CheckProxyModeAndServerMode(const PolicyMap& policies,
                                   PolicyErrorMap* errors,
                                   std::string* mode_value);

  DISALLOW_COPY_AND_ASSIGN(ProxyPolicyHandler);
};

// Handles JavaScript policies.
class JavascriptPolicyHandler : public ConfigurationPolicyHandler {
 public:
  JavascriptPolicyHandler();
  virtual ~JavascriptPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual bool CheckPolicySettings(const PolicyMap& policies,
                                   PolicyErrorMap* errors) OVERRIDE;
  virtual void ApplyPolicySettings(const PolicyMap& policies,
                                   PrefValueMap* prefs) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(JavascriptPolicyHandler);
};

// Handles RestoreOnStartup policy.
class RestoreOnStartupPolicyHandler : public SimplePolicyHandler {
 public:
  RestoreOnStartupPolicyHandler();
  virtual ~RestoreOnStartupPolicyHandler();

  // ConfigurationPolicyHandler methods:
  virtual bool CheckPolicySettings(const PolicyMap& policies,
                                   PolicyErrorMap* errors) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(RestoreOnStartupPolicyHandler);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_CONFIGURATION_POLICY_HANDLER_H_
