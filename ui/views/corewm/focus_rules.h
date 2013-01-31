// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COREWM_FOCUS_RULES_H_
#define UI_VIEWS_COREWM_FOCUS_RULES_H_

#include "ui/views/views_export.h"

namespace aura {
class Window;
}

namespace views {
namespace corewm {

// Implemented by an object that establishes the rules about what can be
// focused or activated.
class VIEWS_EXPORT FocusRules {
 public:
  virtual ~FocusRules() {}

  // Returns true if |window| is a toplevel window. Whether or not a window
  // is considered toplevel is determined by a similar set of rules that
  // govern activation and focus. Not all toplevel windows are activatable,
  // call CanActivateWindow() to determine if a window can be activated.
  virtual bool IsToplevelWindow(aura::Window* window) const = 0;
  // Returns true if |window| can be activated or focused.
  virtual bool CanActivateWindow(aura::Window* window) const = 0;
  // For CanFocusWindow(), NULL is supported, because NULL is a valid focusable
  // window (in the case of clearing focus).
  virtual bool CanFocusWindow(aura::Window* window) const = 0;

  // Returns the toplevel window containing |window|. Not all toplevel windows
  // are activatable, call GetActivatableWindow() instead to return the
  // activatable window, which might be in a different hierarchy.
  // Will return NULL if |window| is not contained by a window considered to be
  // a toplevel window.
  virtual aura::Window* GetToplevelWindow(aura::Window* window) const = 0;
  // Returns the activatable or focusable window given an attempt to activate or
  // focus |window|. Some possible scenarios (not intended to be exhaustive):
  // - |window| is a child of a non-focusable window and so focus must be set
  //   according to rules defined by the delegate, e.g. to a parent.
  // - |window| is an activatable window that is the transient parent of a modal
  //   window, so attempts to activate |window| should result in the modal
  //   transient being activated instead.
  // These methods may return NULL if they are unable to find an activatable
  // or focusable window given |window|.
  virtual aura::Window* GetActivatableWindow(aura::Window* window) const = 0;
  virtual aura::Window* GetFocusableWindow(aura::Window* window) const = 0;

  // Returns the next window to activate in the event that |ignore| is no longer
  // activatable. This function is called when something is happening to
  // |ignore| that means it can no longer have focus or activation, including
  // but not limited to:
  // - it or its parent hierarchy is being hidden, or removed from the
  //   RootWindow.
  // - it is being destroyed.
  // - it is being explicitly deactivated.
  // |ignore| cannot be NULL.
  virtual aura::Window* GetNextActivatableWindow(
      aura::Window* ignore) const = 0;
};

}  // namespace corewm
}  // namespace views

#endif  // UI_VIEWS_COREWM_FOCUS_RULES_H_
