// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_LOGIN_UI_SERVICE_FACTORY_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_LOGIN_UI_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class LoginUIService;
class Profile;

// Singleton that owns all LoginUIServices and associates them with
// Profiles. Listens for the Profile's destruction notification and cleans up
// the associated LoginUIService.
class LoginUIServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the instance of LoginUIService associated with this profile
  // (creating one if none exists). Returns NULL if this profile cannot have a
  // LoginUIService (for example, if |profile| is incognito).
  static LoginUIService* GetForProfile(Profile* profile);

  // Returns an instance of the LoginUIServiceFactory singleton.
  static LoginUIServiceFactory* GetInstance();

 private:
  friend struct DefaultSingletonTraits<LoginUIServiceFactory>;

  LoginUIServiceFactory();
  virtual ~LoginUIServiceFactory();

  // ProfileKeyedServiceFactory:
  virtual ProfileKeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(LoginUIServiceFactory);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_LOGIN_UI_SERVICE_FACTORY_H_
