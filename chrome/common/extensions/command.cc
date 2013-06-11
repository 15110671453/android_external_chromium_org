// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/command.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_manifest_constants.h"
#include "extensions/common/error_utils.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace errors = extension_manifest_errors;
namespace keys = extension_manifest_keys;
namespace values = extension_manifest_values;

using extensions::ErrorUtils;
using extensions::Command;

namespace {

static const char kMissing[] = "Missing";

static const char kCommandKeyNotSupported[] =
    "Command key is not supported. Note: Ctrl means Command on Mac";

ui::Accelerator ParseImpl(const std::string& accelerator,
                          const std::string& platform_key,
                          int index,
                          string16* error) {
  if (platform_key != values::kKeybindingPlatformWin &&
      platform_key != values::kKeybindingPlatformMac &&
      platform_key != values::kKeybindingPlatformChromeOs &&
      platform_key != values::kKeybindingPlatformLinux &&
      platform_key != values::kKeybindingPlatformDefault) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kInvalidKeyBindingUnknownPlatform,
        base::IntToString(index),
        platform_key);
    return ui::Accelerator();
  }

  std::vector<std::string> tokens;
  base::SplitString(accelerator, '+', &tokens);
  if (tokens.size() < 2 || tokens.size() > 3) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kInvalidKeyBinding,
        base::IntToString(index),
        platform_key,
        accelerator);
    return ui::Accelerator();
  }

  // Now, parse it into an accelerator.
  int modifiers = ui::EF_NONE;
  ui::KeyboardCode key = ui::VKEY_UNKNOWN;
  for (size_t i = 0; i < tokens.size(); i++) {
    if (tokens[i] == values::kKeyCtrl) {
      modifiers |= ui::EF_CONTROL_DOWN;
    } else if (tokens[i] == values::kKeyCommand) {
      if (platform_key == values::kKeybindingPlatformMac) {
        // Either the developer specified Command+foo in the manifest for Mac or
        // they specified Ctrl and it got normalized to Command (to get Ctrl on
        // Mac the developer has to specify MacCtrl). Therefore we treat this
        // as Command.
        modifiers |= ui::EF_COMMAND_DOWN;
#if defined(OS_MACOSX)
      } else if (platform_key == values::kKeybindingPlatformDefault) {
        // If we see "Command+foo" in the Default section it can mean two
        // things, depending on the platform:
        // The developer specified "Ctrl+foo" for Default and it got normalized
        // on Mac to "Command+foo". This is fine. Treat it as Command.
        modifiers |= ui::EF_COMMAND_DOWN;
#endif
      } else {
        // No other platform supports Command.
        key = ui::VKEY_UNKNOWN;
        break;
      }
    } else if (tokens[i] == values::kKeyAlt) {
      modifiers |= ui::EF_ALT_DOWN;
    } else if (tokens[i] == values::kKeyShift) {
      modifiers |= ui::EF_SHIFT_DOWN;
    } else if (tokens[i].size() == 1 ||  // A-Z, 0-9.
               tokens[i] == values::kKeyComma ||
               tokens[i] == values::kKeyPeriod ||
               tokens[i] == values::kKeyUp ||
               tokens[i] == values::kKeyDown ||
               tokens[i] == values::kKeyLeft ||
               tokens[i] == values::kKeyRight ||
               tokens[i] == values::kKeyIns ||
               tokens[i] == values::kKeyDel ||
               tokens[i] == values::kKeyHome ||
               tokens[i] == values::kKeyEnd ||
               tokens[i] == values::kKeyPgUp ||
               tokens[i] == values::kKeyPgDwn ||
               tokens[i] == values::kKeyTab) {
      if (key != ui::VKEY_UNKNOWN) {
        // Multiple key assignments.
        key = ui::VKEY_UNKNOWN;
        break;
      }

      if (tokens[i] == values::kKeyComma) {
        key = ui::VKEY_OEM_COMMA;
      } else if (tokens[i] == values::kKeyPeriod) {
        key = ui::VKEY_OEM_PERIOD;
      } else if (tokens[i] == values::kKeyUp) {
        key = ui::VKEY_UP;
      } else if (tokens[i] == values::kKeyDown) {
        key = ui::VKEY_DOWN;
      } else if (tokens[i] == values::kKeyLeft) {
        key = ui::VKEY_LEFT;
      } else if (tokens[i] == values::kKeyRight) {
        key = ui::VKEY_RIGHT;
      } else if (tokens[i] == values::kKeyIns) {
        key = ui::VKEY_INSERT;
      } else if (tokens[i] == values::kKeyDel) {
        key = ui::VKEY_DELETE;
      } else if (tokens[i] == values::kKeyHome) {
        key = ui::VKEY_HOME;
      } else if (tokens[i] == values::kKeyEnd) {
        key = ui::VKEY_END;
      } else if (tokens[i] == values::kKeyPgUp) {
        key = ui::VKEY_PRIOR;
      } else if (tokens[i] == values::kKeyPgDwn) {
        key = ui::VKEY_NEXT;
      } else if (tokens[i] == values::kKeyTab) {
        key = ui::VKEY_TAB;
      } else if (tokens[i].size() == 1 &&
                 tokens[i][0] >= 'A' && tokens[i][0] <= 'Z') {
        key = static_cast<ui::KeyboardCode>(ui::VKEY_A + (tokens[i][0] - 'A'));
      } else if (tokens[i].size() == 1 &&
                 tokens[i][0] >= '0' && tokens[i][0] <= '9') {
        key = static_cast<ui::KeyboardCode>(ui::VKEY_0 + (tokens[i][0] - '0'));
      } else {
        key = ui::VKEY_UNKNOWN;
        break;
      }
    } else {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidKeyBinding,
          base::IntToString(index),
          platform_key,
          accelerator);
      return ui::Accelerator();
    }
  }
  bool command = (modifiers & ui::EF_COMMAND_DOWN) != 0;
  bool ctrl = (modifiers & ui::EF_CONTROL_DOWN) != 0;
  bool alt = (modifiers & ui::EF_ALT_DOWN) != 0;
  bool shift = (modifiers & ui::EF_SHIFT_DOWN) != 0;

  // We support Ctrl+foo, Alt+foo, Ctrl+Shift+foo, Alt+Shift+foo, but not
  // Ctrl+Alt+foo and not Shift+foo either. For a more detailed reason why we
  // don't support Ctrl+Alt+foo see this article:
  // http://blogs.msdn.com/b/oldnewthing/archive/2004/03/29/101121.aspx.
  // On Mac Command can also be used in combination with Shift or on its own,
  // as a modifier.
  if (key == ui::VKEY_UNKNOWN || (ctrl && alt) || (command && alt) ||
      (shift && !ctrl && !alt && !command)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kInvalidKeyBinding,
        base::IntToString(index),
        platform_key,
        accelerator);
    return ui::Accelerator();
  }

  return ui::Accelerator(key, modifiers);
}

// For Mac, we convert "Ctrl" to "Command" and "MacCtrl" to "Ctrl". Other
// platforms leave the shortcut untouched.
std::string NormalizeShortcutSuggestion(const std::string& suggestion,
                                        const std::string& platform) {
  bool normalize = false;
  if (platform == values::kKeybindingPlatformMac) {
    normalize = true;
  } else if (platform == values::kKeybindingPlatformDefault) {
#if defined(OS_MACOSX)
    normalize = true;
#endif
  }

  if (!normalize)
    return suggestion;

  std::string key;
  std::vector<std::string> tokens;
  base::SplitString(suggestion, '+', &tokens);
  for (size_t i = 0; i < tokens.size(); i++) {
    if (tokens[i] == values::kKeyCtrl)
      tokens[i] = values::kKeyCommand;
    else if (tokens[i] == values::kKeyMacCtrl)
      tokens[i] = values::kKeyCtrl;
  }
  return JoinString(tokens, '+');
}

}  // namespace

namespace extensions {

Command::Command() {}

Command::Command(const std::string& command_name,
                 const string16& description,
                 const std::string& accelerator)
    : command_name_(command_name),
      description_(description) {
  string16 error;
  accelerator_ = ParseImpl(accelerator, CommandPlatform(), 0, &error);
}

Command::~Command() {}

// static
std::string Command::CommandPlatform() {
#if defined(OS_WIN)
  return values::kKeybindingPlatformWin;
#elif defined(OS_MACOSX)
  return values::kKeybindingPlatformMac;
#elif defined(OS_CHROMEOS)
  return values::kKeybindingPlatformChromeOs;
#elif defined(OS_LINUX)
  return values::kKeybindingPlatformLinux;
#else
  return "";
#endif
}

// static
ui::Accelerator Command::StringToAccelerator(const std::string& accelerator) {
  string16 error;
  Command command;
  ui::Accelerator parsed =
      ParseImpl(accelerator, Command::CommandPlatform(), 0, &error);
  return parsed;
}

// static
std::string Command::AcceleratorToString(const ui::Accelerator& accelerator) {
  std::string shortcut;

  // Ctrl and Alt are mutually exclusive.
  if (accelerator.IsCtrlDown())
    shortcut += values::kKeyCtrl;
  else if (accelerator.IsAltDown())
    shortcut += values::kKeyAlt;
  if (!shortcut.empty())
    shortcut += values::kKeySeparator;

  if (accelerator.IsCmdDown()) {
    shortcut += values::kKeyCommand;
    shortcut += values::kKeySeparator;
  }

  if (accelerator.IsShiftDown()) {
    shortcut += values::kKeyShift;
    shortcut += values::kKeySeparator;
  }

  if (accelerator.key_code() >= ui::VKEY_0 &&
      accelerator.key_code() <= ui::VKEY_9) {
    shortcut += '0' + (accelerator.key_code() - ui::VKEY_0);
  } else if (accelerator.key_code() >= ui::VKEY_A &&
           accelerator.key_code() <= ui::VKEY_Z) {
    shortcut += 'A' + (accelerator.key_code() - ui::VKEY_A);
  } else {
    switch (accelerator.key_code()) {
      case ui::VKEY_OEM_COMMA:
        shortcut += values::kKeyComma;
        break;
      case ui::VKEY_OEM_PERIOD:
        shortcut += values::kKeyPeriod;
        break;
      case ui::VKEY_UP:
        shortcut += values::kKeyUp;
        break;
      case ui::VKEY_DOWN:
        shortcut += values::kKeyDown;
        break;
      case ui::VKEY_LEFT:
        shortcut += values::kKeyLeft;
        break;
      case ui::VKEY_RIGHT:
        shortcut += values::kKeyRight;
        break;
      case ui::VKEY_INSERT:
        shortcut += values::kKeyIns;
        break;
      case ui::VKEY_DELETE:
        shortcut += values::kKeyDel;
        break;
      case ui::VKEY_HOME:
        shortcut += values::kKeyHome;
        break;
      case ui::VKEY_END:
        shortcut += values::kKeyEnd;
        break;
      case ui::VKEY_PRIOR:
        shortcut += values::kKeyPgUp;
        break;
      case ui::VKEY_NEXT:
        shortcut += values::kKeyPgDwn;
        break;
      case ui::VKEY_TAB:
        shortcut += values::kKeyTab;
        break;
      default:
        return "";
    }
  }
  return shortcut;
}

bool Command::Parse(const DictionaryValue* command,
                    const std::string& command_name,
                    int index,
                    string16* error) {
  DCHECK(!command_name.empty());

  string16 description;
  if (command_name != values::kPageActionCommandEvent &&
      command_name != values::kBrowserActionCommandEvent &&
      command_name != values::kScriptBadgeCommandEvent) {
    if (!command->GetString(keys::kDescription, &description) ||
        description.empty()) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidKeyBindingDescription,
          base::IntToString(index));
      return false;
    }
  }

  // We'll build up a map of platform-to-shortcut suggestions.
  typedef std::map<const std::string, std::string> SuggestionMap;
  SuggestionMap suggestions;

  // First try to parse the |suggested_key| as a dictionary.
  const DictionaryValue* suggested_key_dict;
  if (command->GetDictionary(keys::kSuggestedKey, &suggested_key_dict)) {
    for (DictionaryValue::Iterator iter(*suggested_key_dict); !iter.IsAtEnd();
         iter.Advance()) {
      // For each item in the dictionary, extract the platforms specified.
      std::string suggested_key_string;
      if (iter.value().GetAsString(&suggested_key_string) &&
          !suggested_key_string.empty()) {
        // Found a platform, add it to the suggestions list.
        suggestions[iter.key()] = suggested_key_string;
      } else {
        *error = ErrorUtils::FormatErrorMessageUTF16(
            errors::kInvalidKeyBinding,
            base::IntToString(index),
            keys::kSuggestedKey,
            kMissing);
        return false;
      }
    }
  } else {
    // No dictionary was found, fall back to using just a string, so developers
    // don't have to specify a dictionary if they just want to use one default
    // for all platforms.
    std::string suggested_key_string;
    if (command->GetString(keys::kSuggestedKey, &suggested_key_string) &&
        !suggested_key_string.empty()) {
      // If only a single string is provided, it must be default for all.
      suggestions[values::kKeybindingPlatformDefault] = suggested_key_string;
    } else {
      suggestions[values::kKeybindingPlatformDefault] = "";
    }
  }

  // Normalize the suggestions.
  for (SuggestionMap::iterator iter = suggestions.begin();
       iter != suggestions.end(); ++iter) {
    // Before we normalize Ctrl to Command we must detect when the developer
    // specified Command in the Default section, which will work on Mac after
    // normalization but only fail on other platforms when they try it out on
    // other platforms, which is not what we want.
    if (iter->first == values::kKeybindingPlatformDefault &&
        iter->second.find("Command+") != std::string::npos) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidKeyBinding,
          base::IntToString(index),
          keys::kSuggestedKey,
          kCommandKeyNotSupported);
      return false;
    }

    suggestions[iter->first] = NormalizeShortcutSuggestion(iter->second,
                                                           iter->first);
  }

  std::string platform = CommandPlatform();
  std::string key = platform;
  if (suggestions.find(key) == suggestions.end())
    key = values::kKeybindingPlatformDefault;
  if (suggestions.find(key) == suggestions.end()) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kInvalidKeyBindingMissingPlatform,
        base::IntToString(index),
        keys::kSuggestedKey,
        platform);
    return false;  // No platform specified and no fallback. Bail.
  }

  // For developer convenience, we parse all the suggestions (and complain about
  // errors for platforms other than the current one) but use only what we need.
  std::map<const std::string, std::string>::const_iterator iter =
      suggestions.begin();
  for ( ; iter != suggestions.end(); ++iter) {
    ui::Accelerator accelerator;
    if (!iter->second.empty()) {
      // Note that we pass iter->first to pretend we are on a platform we're not
      // on.
      accelerator = ParseImpl(iter->second, iter->first, index, error);
      if (accelerator.key_code() == ui::VKEY_UNKNOWN) {
        *error = ErrorUtils::FormatErrorMessageUTF16(
            errors::kInvalidKeyBinding,
            base::IntToString(index),
            iter->first,
            iter->second);
        return false;
      }
    }

    if (iter->first == key) {
      // This platform is our platform, so grab this key.
      accelerator_ = accelerator;
      command_name_ = command_name;
      description_ = description;
    }
  }
  return true;
}

DictionaryValue* Command::ToValue(const Extension* extension,
                                  bool active) const {
  DictionaryValue* extension_data = new DictionaryValue();

  string16 command_description;
  if (command_name() == values::kBrowserActionCommandEvent ||
      command_name() == values::kPageActionCommandEvent ||
      command_name() == values::kScriptBadgeCommandEvent) {
    command_description =
        l10n_util::GetStringUTF16(IDS_EXTENSION_COMMANDS_GENERIC_ACTIVATE);
  } else {
    command_description = description();
  }
  extension_data->SetString("description", command_description);
  extension_data->SetBoolean("active", active);
  extension_data->SetString("keybinding", accelerator().GetShortcutText());
  extension_data->SetString("command_name", command_name());
  extension_data->SetString("extension_id", extension->id());

  return extension_data;
}

}  // namespace extensions
