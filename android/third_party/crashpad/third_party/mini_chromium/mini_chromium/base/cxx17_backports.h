// Copyright 2006-2008 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MINI_CHROMIUM_BASE_CXX17_BACKPORTS_H_
#define MINI_CHROMIUM_BASE_CXX17_BACKPORTS_H_

#include <sys/types.h>

namespace base {

template <typename T, size_t N>
constexpr size_t size(const T (&array)[N]) noexcept {
  return N;
}

}  // namespace base

#endif  // MINI_CHROMIUM_BASE_CXX17_BACKPORTS_H_
