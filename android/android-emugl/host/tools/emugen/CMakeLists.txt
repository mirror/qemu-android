cmake_minimum_required(VERSION 3.5)
project(emugen)

if (NOT ANDROID_QEMU2_TOP_DIR AND MSVC)
  get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../" ABSOLUTE)
  get_filename_component(ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/" ABSOLUTE)
  list (APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
  include(visual-studio)
  include(android)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/android/msvc-posix-compat posix)
endif()

include(android)

if(NOT TARGET gmock_main)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../googletest googletest)
  # Okay, google test is not happy under msvc
  android_target_compile_definitions(gmock_main windows_msvc-x86_64 PUBLIC
                                     -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
  android_target_compile_definitions(gmock windows_msvc-x86_64 PUBLIC
                                     -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
  android_target_compile_definitions(gtest windows_msvc-x86_64 PUBLIC
                                     -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
endif()

# Emugen executable (note not unicode safe!)
set(emugen_src ApiGen.cpp EntryPoint.cpp main.cpp Parser.cpp strUtils.cpp TypeFactory.cpp)
android_add_executable(emugen)
target_include_directories(emugen PRIVATE ${ANDROID_QEMU2_TOP_DIR}/android/android-emu)
android_target_link_libraries(emugen windows_msvc-x86_64 PRIVATE msvc-posix-compat)

# The unit tests
set(emugen_unittests_src Parser.cpp Parser_unittest.cpp)
android_add_test(emugen_unittests)
target_link_libraries(emugen_unittests PRIVATE gmock_main)
android_target_link_libraries(emugen windows_msvc-x86_64 PRIVATE msvc-posix-compat)