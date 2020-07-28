// Copyright (c) 2019-2020 LG Electronics, Inc.
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
//
// SPDX-License-Identifier: Apache-2.0

#ifndef COMMANDTYPES_H_
#define COMMANDTYPES_H_

#include <string>
#include "pbnjson.hpp"
#include "PdmErrors.h"

enum DeviceCommand { FORMAT = 0,
                     EJECT,
                     FSCK,
                     SET_VOLUME_LABEL,
                     UMOUNT_ALL_DRIVE,
                     IS_WRITABLE_DRIVE,
                     MOUNT_FSCK,
                     SPACE_INFO,
                     IO_PERFORMANCE,
                     NONE
};

typedef struct CommandType {
    DeviceCommand commandId;
}CommandType;

typedef struct EjectCommand {
    const DeviceCommand commandId = EJECT;
    int deviceNumber;
}EjectCommand;

typedef struct FsckCommand {
    const DeviceCommand commandId = FSCK;
    std::string driveName;
}FsckCommand;

typedef struct FormatCommand {
    const DeviceCommand commandId = FORMAT;
    std::string driveName;
    std::string fsType;
    std::string volumeLabel;
}FormatCommand;

typedef struct VolumeLabelCommand {
    const DeviceCommand commandId = SET_VOLUME_LABEL;
    std::string driveName;
    std::string volumeLabel;
}VolumeLabelCommand;

typedef struct UmountAllCommand {
    const DeviceCommand commandId = UMOUNT_ALL_DRIVE;
}UmountAllCommand;

typedef struct IsWritableCommand {
    const DeviceCommand commandId = IS_WRITABLE_DRIVE;
    std::string driveName;
}IsWritableCommand;

typedef struct MountFsckCommand {
    const DeviceCommand commandId = MOUNT_FSCK;
    bool needFsck;
    std::string mountName;
}MountFsckCommand;

typedef struct SpaceInfoCommand {
    const DeviceCommand commandId = SPACE_INFO;
    bool directCheck;
    std::string driveName;
}SpaceInfoCommand;

typedef struct CommandResponse {
    pbnjson::JValue cmdResponse;
}CommandResponse;

#endif /* COMMANDTYPES_H_ */
