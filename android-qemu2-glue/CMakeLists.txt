cmake_minimum_required(VERSION 3.5)

android_add_library(
  TARGET android-wifi-service
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      emulation/virtio-wifi.cpp
      emulation/WifiService.cpp
      emulation/VirtioWifiForwarder.cpp
      emulation/NetsimWifiForwarder.cpp)

target_include_directories(
  android-wifi-service
  PRIVATE ..
          ../include
          ${ANDROID_AUTOGEN}
          ../slirp)

target_link_libraries(
  android-wifi-service
  PRIVATE android-emu
          GLIB2::GLIB2
          ssl
          hostapd
          android-grpc
          qemu-host-common-headers
          packet-streamer-client-lib
          packet-streamer-proto-lib
          protobuf::libprotobuf)

# Generate the VirtioGpuTimelines flatbuffers header
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/VirtioGpuTimelines_generated.h
  COMMAND flatc ARGS --cpp --gen-object-api --scoped-enums -o
          ${CMAKE_CURRENT_BINARY_DIR} emulation/VirtioGpuTimelines.fbs
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS flatc emulation/VirtioGpuTimelines.fbs)
add_custom_target(
  virtio_gpu_timelines_fbs_header ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/VirtioGpuTimelines_generated.h)

set(android-qemu2-glue_src
    android_qemud.cpp
    audio-capturer.cpp
    audio-output.cpp
    base/async/CpuLooper.cpp
    base/async/Looper.cpp
    base/files/QemuFileStream.cpp
    display.cpp
    drive-share.cpp
    dtb.cpp
    emulation/CharSerialLine.cpp
    emulation/DmaMap.cpp
    emulation/VmLock.cpp
    emulation/android_address_space_device.cpp
    emulation/android_pipe_device.cpp
    emulation/charpipe.c
    emulation/goldfish_sync.cpp
    emulation/serial_line.cpp
    emulation/virtio-goldfish-pipe.cpp
    emulation/virtio-input-multi-touch.c
    emulation/virtio-input-rotary.c
    emulation/virtio_vsock_device.cpp
    emulation/virtio_vsock_transport.cpp
    emulation/VirtioGpuTimelines.cpp
    looper-qemu.cpp
    net-android.cpp
    proxy/slirp_proxy.cpp
    qemu-automation-agent-impl.cpp
    qemu-battery-agent-impl.cpp
    qemu-car-data-agent-impl.cpp
    qemu-cellular-agent-impl.c
    qemu-console-factory.cpp
    qemu-clipboard-agent-impl.cpp
    qemu-display-agent-impl.cpp
    qemu-finger-agent-impl.c
    qemu-globals-agent-impl.cpp
    qemu-grpc-agent-impl.cpp
    qemu-http-proxy-agent-impl.c
    qemu-hw-control-agent-impl.cpp
    qemu-location-agent-impl.c
    qemu-multi-display-agent-impl.cpp
    qemu-net-agent-impl.c
    qemu-record-screen-agent-impl.c
    qemu-sensors-agent-impl.cpp
    qemu-setup-dns-servers.cpp
    qemu-setup.cpp
    qemu-rootcanal-agent-impl.cpp
    netsim/BluetoothPacketProtocol.cpp
    netsim/PacketStreamTransport.cpp
    netsim/qemu-packet-stream-agent-impl.cpp
    qemu-telephony-agent-impl.c
    qemu-user-event-agent-impl.c
    qemu-virtual-scene-agent-impl.cpp
    snapshot_compression.cpp
    telephony/modem_init.c
    utils/stream.cpp)

android_add_library(
  TARGET libqemu2-glue LICENSE Apache-2.0 SRC # cmake-format: sortable
                                              ${android-qemu2-glue_src})

target_include_directories(
  libqemu2-glue
  PRIVATE ..
          ../include
          ${ANDROID_AUTOGEN}
          ../slirp
          ${VIRGLRENDERER_INCLUDE_DIR}
          ${ANDROID_QEMU2_TOP_DIR}/android/third_party/libdtb/include/
          ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(libqemu2-glue PRIVATE android-grpc android-ui-controller)
if(WEBRTC)
  target_link_libraries(libqemu2-glue PRIVATE android-webrtc)
endif()
if(BLUETOOTH_EMULATION)
  target_link_libraries(
    libqemu2-glue
    PRIVATE grpc-emulated-bluetooth
            grpc++
            packet-streamer-client-lib
            packet-streamer-proto-lib
            protobuf::libprotobuf
            libbt-rootcanal
            libbt-rootcanal-qemu)
endif()

target_link_libraries(
  libqemu2-glue
  PRIVATE android-emu
          android-wifi-service
          GLIB2::GLIB2
          PIXMAN::PIXMAN
          emulator-libdtb
          android-emu-gps
          ssl
          flatbuffers
          qemu-host-common-headers
          grpc++
          packet-streamer-client-lib
          packet-streamer-proto-lib
          protobuf::libprotobuf)

add_dependencies(libqemu2-glue virtio_gpu_timelines_fbs_header)

target_compile_definitions(libqemu2-glue INTERFACE "-DCONFIG_VIRGL")
android_add_library(
  TARGET libqemu2-glue-vm-operations LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${ANDROID_HW_CONFIG_H} qemu-vm-operations-impl.cpp)
target_include_directories(libqemu2-glue-vm-operations
                           PRIVATE .. ../include ${ANDROID_AUTOGEN})
target_link_libraries(
  libqemu2-glue-vm-operations
  PRIVATE android-emu android-emu-base-headers GLIB2::GLIB2 android-hw-config
          nlohmann_json::nlohmann_json PIXMAN::PIXMAN)

android_add_test(TARGET libqemu2-glue_unittests
                 SRC tests/VirtioGpuTimelines_unittest.cpp)

target_link_libraries(
  libqemu2-glue_unittests
  PRIVATE libqemu2-glue
          qemu-host-common-headers
          android-emu-base
          android-emu-base-headers
          gmock
          gtest
          gmock_main)

target_include_directories(
  libqemu2-glue_unittests
  PRIVATE . ../include ../android/android-emu-base ../android/android-emu
          ${VIRGLRENDERER_INCLUDE_DIR})
