// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/PathUtils.h"

#include "android/avd/info.h"
#include "android/base/files/PathUtils.h"
#include "android/globals.h"

namespace android {
namespace snapshot {

std::string getSnapshotDir(const char* snapshotName) {
    auto dir = avdInfo_getContentPath(android_avdInfo);
    auto path = base::PathUtils::join(dir, "snapshots", snapshotName);
    return path;
}

}  // namespace snapshot
}  // namespace android
