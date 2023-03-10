//
// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>   // for microse...
#include <cstdio>   // for fprintf
#include <ratio>    // for ratio
#include <string>   // for basic_s...
#include <utility>  // for move

#include "aemu/base/Log.h"
#include "android/utils/path.h"
#include "android/base/system/System.h"                       // for System
#include "android/emulation/control/EmulatorAdvertisement.h"

using android::base::System;
using android::emulation::control::EmulatorProperties;
using android::emulation::control::EmulatorAdvertisement;

int main(int argc, char* argv[]) {
    EmulatorProperties cfg({{"hello", "world"}});
    std::unique_ptr<EmulatorAdvertisement> discovery;
    if (argc < 2) {
        discovery = std::make_unique<EmulatorAdvertisement>(std::move(cfg));
        auto status = discovery->write();
        LOG(INFO) <<  (status ? "Successfully Wrote: " : "Failed to write: ") << discovery->location();
    } else {
        auto path = argv[1];
        discovery = std::make_unique<EmulatorAdvertisement>(std::move(cfg), path);
        auto status = discovery->write();
        LOG(INFO) <<  (status ? "Successfully Wrote: " : "Failed to write: ") << discovery->location();
    }

    std::chrono::seconds awhile(300);
    System::get()->sleepMs(std::chrono::milliseconds(awhile).count());
    return 0;
}
