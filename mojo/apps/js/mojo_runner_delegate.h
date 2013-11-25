// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_APPS_JS_MOJO_RUNNER_DELEGATE_H_
#define MOJO_APPS_JS_MOJO_RUNNER_DELEGATE_H_

#include "base/compiler_specific.h"
#include "gin/modules/module_runner_delegate.h"

namespace mojo {
namespace apps {

class MojoRunnerDelegate : public gin::ModuleRunnerDelegate {
 public:
  MojoRunnerDelegate();
  virtual ~MojoRunnerDelegate();

 private:
  // From ModuleRunnerDelegate:
  virtual void UnhandledException(gin::Runner* runner,
                                  gin::TryCatch& try_catch) OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(MojoRunnerDelegate);
};

}  // namespace apps
}  // namespace mojo

#endif  // MOJO_APPS_JS_MOJO_RUNNER_DELEGATE_H_
