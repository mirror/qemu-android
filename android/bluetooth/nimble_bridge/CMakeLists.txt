# BLE Heart Rate peripheral app.
android_add_executable(
  TARGET nimble_bridge INSTALL .
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      android/emulation/bluetooth/main.cpp
      android/emulation/bluetooth/nimble/ServiceDefinition.cpp
      android/emulation/bluetooth/nimble/TypeTranslator.cpp
      android/emulation/bluetooth/RemoteConnection.cpp
      android/emulation/bluetooth/RequestForwarder.cpp
  DEPS android-emu-base-headers)
target_link_libraries(
  nimble_bridge
  PUBLIC nimble_bluetooth
         grpc-emulated-bluetooth
         grpc++
         absl::status
         absl::statusor
         absl::flags
         absl::flags_parse
         android-emu)
target_include_directories(nimble_bridge PUBLIC .)
# C++20 re-introduces this support.
target_compile_options(nimble_bridge PRIVATE -Wno-c99-designator)

android_license(
  TARGET "nimble_bluetooth"
  LIBNAME mynewt-nmble
  SPDX "Apache=2.0"
  LICENSE
    "https://android.googlesource.com/platform/external/qemu/+/emu-master-dev/android/third_party/mynewt-nimble/LICENSE"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/android/third_party/mynewt-nimble/NOTICE")
