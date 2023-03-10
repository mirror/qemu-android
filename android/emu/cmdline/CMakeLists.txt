android_add_library(
  TARGET android-emu-cmdline
  # SHARED # For testing only.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/cmdline-option.cpp android/emulation/bufprint_config_dirs.cpp
      android/emulation/ParameterList.cpp android/help.c
      android/network/constants.c
  DEPS android-emu-base android-emu-base-headers android-emu-files)
target_include_directories(
  android-emu-cmdline
  PRIVATE src
  PUBLIC
    include
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG})

if(BLUETOOTH_EMULATION)
  target_compile_definitions(android-emu-cmdline PRIVATE -DANDROID_BLUETOOTH)
endif()

if(WEBRTC)
  target_compile_definitions(android-emu-cmdline PRIVATE -DANDROID_WEBRTC)
endif()

android_add_test(
  TARGET android-emu-cmdline_unittests
  SOURCE_DIR test
  SRC android/cmdline-option_unittest.cpp
      android/emulation/bufprint_config_dirs_unittest.cpp
      android/network/constants_unittest.cpp)
target_link_libraries(android-emu-cmdline_unittests
                      PRIVATE android-emu-cmdline android-emu-base android-emu-base-headers
                      gmock_main)


add_library(android-emu-cmdline-testing INTERFACE)
target_include_directories(android-emu-cmdline-testing INTERFACE test)
