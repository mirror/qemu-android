// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MINI_CHROMIUM_BASE_FILES_SCOPED_FILE_H_
#define MINI_CHROMIUM_BASE_FILES_SCOPED_FILE_H_

#include <stdio.h>

#include <memory>

#include "base/scoped_generic.h"
#include "build/build_config.h"

namespace base {

namespace internal {

#if BUILDFLAG(IS_POSIX)
struct ScopedFDCloseTraits {
  static int InvalidValue() {
    return -1;
  }
  static void Free(int fd);
};
#endif  // BUILDFLAG(IS_POSIX)

struct ScopedFILECloser {
  void operator()(FILE* file) const;
};

}  // namespace internal

#if BUILDFLAG(IS_POSIX)
typedef ScopedGeneric<int, internal::ScopedFDCloseTraits> ScopedFD;
#endif  // BUILDFLAG(IS_POSIX)
typedef std::unique_ptr<FILE, internal::ScopedFILECloser> ScopedFILE;

}  // namespace base

#endif  // MINI_CHROMIUM_BASE_FILES_SCOPED_FILE_H_
