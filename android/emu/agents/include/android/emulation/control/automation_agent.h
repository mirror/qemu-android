// Copyright (C) 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once

#include "android/automation/AutomationController.h"
#include "android/utils/compiler.h"

#include <string_view>

ANDROID_BEGIN_HEADER

using namespace android::base;

typedef struct QAndroidAutomationAgent {
    // Reset the current state and cancel any recordings or playback.
    // Called on snapshot restore, since playback cannot be trivially resumed.
    void (*reset)();

    // Start a recording to a file.
    android::automation::StartResult (*startRecording)(
            std::string_view filename);

    // Stops a recording to a file.
    android::automation::StopResult (*stopRecording)();

    // Start a playback from a file.
    android::automation::StartResult (*startPlayback)(
            std::string_view filename);

    // Stop playback from a file.
    android::automation::StopResult (*stopPlayback)();

    // Start playback with stop callback.
    android::automation::StartResult (*startPlaybackWithCallback)(
            std::string_view filename,
            void (*onStopCallback)());

    // Set the macro name in the header of a file.
    void (*setMacroName)(std::string_view macroName, std::string_view filename);

    // Get the macro name from the header of a file.
    std::string (*getMacroName)(std::string_view filename);

    // Get the duration in ns and datetime in ms from a file.
    std::pair<uint64_t, uint64_t> (*getMetadata)(std::string_view filename);
} QAndroidAutomationAgent;

ANDROID_END_HEADER
