// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROS_SCREEN_LOCK_LIBRARY_H_
#define CHROME_BROWSER_CHROMEOS_CROS_SCREEN_LOCK_LIBRARY_H_

#include "base/observer_list.h"
#include "third_party/cros/chromeos_screen_lock.h"

namespace chromeos {

// This interface defines interaction with the ChromeOS screen lock
// APIs.
class ScreenLockLibrary {
 public:
  class Observer {
   public:
    virtual ~Observer() {}
    virtual void ScreenLocked(ScreenLockLibrary* obj) = 0;
  };
  ScreenLockLibrary() {}
  virtual ~ScreenLockLibrary() {}
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Notifies PowerManager that screen lock has been completed.
  virtual void NotifyScreenLockCompleted() = 0;
  // Notifies PowerManager that a user requested to lock the screen.
  virtual void NotifyScreenLockRequested() = 0;
  // Notifies PowerManager that a user unlocked the screen.
  virtual void NotifyScreenUnlocked() = 0;
};

// This class handles the interaction with the ChromeOS screen lock APIs.
class ScreenLockLibraryImpl : public ScreenLockLibrary {
 public:
  ScreenLockLibraryImpl();
  virtual ~ScreenLockLibraryImpl();

  // ScreenLockLibrary implementations:
  virtual void AddObserver(Observer* observer);
  virtual void RemoveObserver(Observer* observer);
  virtual void NotifyScreenLockCompleted();
  virtual void NotifyScreenLockRequested();
  virtual void NotifyScreenUnlocked();

 private:
  // This method is called when PowerManager requests to lock the screen.
  // This method is called on a background thread.
  static void ScreenLockedHandler(void* object);

  // This methods starts the monitoring of screen lock request.
  void Init();

  // Called by the handler to notify the screen lock request from
  // PowerManager.
  void ScreenLocked();

  ObserverList<Observer> observers_;

  // A reference to the screen lock api
  chromeos::ScreenLockConnection screen_lock_connection_;

  DISALLOW_COPY_AND_ASSIGN(ScreenLockLibraryImpl);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CROS_SCREEN_LOCK_LIBRARY_H_
