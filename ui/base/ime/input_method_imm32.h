// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_INPUT_METHOD_IMM32_H_
#define UI_BASE_IME_INPUT_METHOD_IMM32_H_

#include <windows.h>

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ui/base/ime/input_method_win.h"

namespace ui {

// An InputMethod implementation based on Windows IMM32 API.
class UI_EXPORT InputMethodIMM32 : public InputMethodWin {
 public:
  InputMethodIMM32(internal::InputMethodDelegate* delegate,
                   HWND toplevel_window_handle);

  // Overridden from InputMethod:
  virtual void OnFocus() OVERRIDE;
  virtual void OnBlur() OVERRIDE;
  virtual bool OnUntranslatedIMEMessage(const base::NativeEvent& event,
                                        NativeEventResult* result) OVERRIDE;
  virtual void OnTextInputTypeChanged(const TextInputClient* client) OVERRIDE;
  virtual void OnCaretBoundsChanged(const TextInputClient* client) OVERRIDE;
  virtual void CancelComposition(const TextInputClient* client) OVERRIDE;
  virtual void SetFocusedTextInputClient(TextInputClient* client) OVERRIDE;

 protected:
  // Overridden from InputMethodBase:
  virtual void OnWillChangeFocusedClient(TextInputClient* focused_before,
                                         TextInputClient* focused) OVERRIDE;
  virtual void OnDidChangeFocusedClient(TextInputClient* focused_before,
                                        TextInputClient* focused) OVERRIDE;

 private:
  LRESULT OnImeSetContext(UINT message,
                          WPARAM wparam,
                          LPARAM lparam,
                          BOOL* handled);
  LRESULT OnImeStartComposition(UINT message,
                                WPARAM wparam,
                                LPARAM lparam,
                                BOOL* handled);
  LRESULT OnImeComposition(UINT message,
                           WPARAM wparam,
                           LPARAM lparam,
                           BOOL* handled);
  LRESULT OnImeEndComposition(UINT message,
                              WPARAM wparam,
                              LPARAM lparam,
                              BOOL* handled);

  // Asks the client to confirm current composition text.
  void ConfirmCompositionText();

  // Enables or disables the IME according to the current text input type.
  void UpdateIMEState();

  bool enabled_;

  DISALLOW_COPY_AND_ASSIGN(InputMethodIMM32);
};

}  // namespace ui

#endif  // UI_BASE_IME_INPUT_METHOD_IMM32_H_
