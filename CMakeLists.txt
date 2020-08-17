# Copyright (c) 2019-2020 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.7)
project(physical-device-manager)
include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC4)
webos_component(1 0 1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")

# add include files
include_directories(${CMAKE_SOURCE_DIR}/inc)
include_directories(${CMAKE_SOURCE_DIR}/inc/private)
include_directories(${CMAKE_SOURCE_DIR}/inc/public)

#  find required packages
include(FindPkgConfig)

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS_OTHER})

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
#include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS})

pkg_check_modules(LUNASERVICE REQUIRED luna-service2)
include_directories(${LUNASERVICE_INCLUDE_DIR})
webos_add_compiler_flags(ALL ${LUNASERVICE_CFLAGS})

pkg_check_modules(LS2 REQUIRED luna-service2++)
include_directories(${LS2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2_CFLAGS})

pkg_check_modules(PMLOG REQUIRED PmLogLib)
include_directories(${PMLOG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOG_CFLAGS_OTHER})

pkg_check_modules(WEBOSI18N REQUIRED webosi18n)
include_directories(${WEBOSI18N_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${WEBOSI18N_CFLAGS_OTHER})

pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
include_directories(${LIBUSB_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LIBUSB_CFLAGS_OTHER})

if(WEBOS_TARGET_DISTRO STREQUAL "webos-auto")
    if(WEBOS_TARGET_MACHINE STREQUAL "sa8155" )
        add_definitions(-DWEBOS_SESSION)
    endif()
endif()

pkg_check_modules(UDEV REQUIRED libudev)

set(BIN_NAME physical-device-manager)

file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR} *.cpp)

add_executable(${BIN_NAME} ${SRC_FILES})

set(LIB_LIST
    ${UDEV_LDFLAGS}
    ${PBNJSON_CPP_LDFLAGS}
    ${GLIB2_LDFLAGS}
    ${LUNASERVICE_LDFLAGS}
    ${PMLOG_LDFLAGS}
    ${WEBOSI18N_LDFLAGS}
    ${LIBUSB_LDFLAGS}
    ${LS2_LDFLAGS}
    pthread
    -lstdc++fs
    dl
   )

target_link_libraries (${BIN_NAME} ${LIB_LIST})

webos_build_system_bus_files()
webos_include_install_paths()
webos_build_db8_files()
webos_build_configured_file(files/systemd/physical-device-manager.service SYSCONFDIR systemd/system)

install(TARGETS ${BIN_NAME} DESTINATION sbin)
install(FILES @CMAKE_SOURCE_DIR@/files/rules/39-usb-auto-port.rules DESTINATION @WEBOS_INSTALL_SYSCONFDIR@/udev/rules.d/)
install(DIRECTORY @CMAKE_SOURCE_DIR@/files/rules/ DESTINATION @WEBOS_INSTALL_SYSCONFDIR@/udev/rules.d/ FILES_MATCHING PATTERN "*.rules*" PATTERN ".*" EXCLUDE)
install(DIRECTORY @CMAKE_SOURCE_DIR@/files/icon/ DESTINATION @WEBOS_INSTALL_DATADIR@/@CMAKE_PROJECT_NAME@ FILES_MATCHING PATTERN "*.png*" PATTERN ".*" EXCLUDE)
install(DIRECTORY "inc/public/" DESTINATION @WEBOS_INSTALL_INCLUDEDIR@ FILES_MATCHING PATTERN "*.h*" PATTERN ".*" EXCLUDE)
install(DIRECTORY @CMAKE_SOURCE_DIR@/files/scripts/ DESTINATION @WEBOS_INSTALL_SYSCONFDIR@/pdm/scripts/ FILES_MATCHING PATTERN "*.sh*" PATTERN ".*" EXCLUDE)
