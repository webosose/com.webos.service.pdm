// Copyright (c) 2019-2023 LG Electronics, Inc.
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

#ifndef SCHEMAVALIDATIONAPI_H
#define SCHEMAVALIDATIONAPI_H

#define JSON_SCHEMA_VALIDATE_ATTACH_DEVICE_LIST \
     SCHEMA_V2_1( \
        ",\"required\":[\"subscribe\"]" , \
         SCHEMA_V2_PROP(subscribe, boolean) \
    )

#define JSON_SCHEMA_FORMAT_VALIDATE_DRIVE_NAME \
     SCHEMA_V2_3( \
         ",\"required\":[\"driveName\"]" , \
          SCHEMA_V2_PROP(driveName, string), \
          SCHEMA_V2_PROP(fsType, string), \
          SCHEMA_V2_PROP(volumeLabel, string) \
    )

#define JSON_SCHEMA_VALIDATE_DRIVE_NAME_VOLUME_LABEL \
     SCHEMA_V2_2( \
         ",\"required\":[\"driveName\",\"volumeLabel\"]" , \
          SCHEMA_V2_PROP(driveName, string), \
          SCHEMA_V2_PROP(volumeLabel, string) \
    )

#define JSON_SCHEMA_VALIDATE_DEVICE_NUMBER \
     SCHEMA_V2_1( \
         ",\"required\":[\"deviceNum\"]" , \
          SCHEMA_V2_PROP(deviceNum, integer) \
    )

#define JSON_SCHEMA_GET_SPACE_INFO_VALIDATE_DRIVE_NAME \
     SCHEMA_V2_2( \
         ",\"required\":[\"driveName\"]" , \
          SCHEMA_V2_PROP(driveName, string), \
          SCHEMA_V2_PROP(directCheck, boolean) \
    )

#define JSON_SCHEMA_VALIDATE_DRIVE_NAME \
     SCHEMA_V2_1( \
         ",\"required\":[\"driveName\"]" , \
          SCHEMA_V2_PROP(driveName, string) \
    )

#define JSON_SCHEMA_VALIDATE_NON_STORAGE_ATTACH_DEVICE_LIST \
     SCHEMA_V2_3( \
        ",\"required\":[\"subscribe\"]" , \
         SCHEMA_V2_PROP(subscribe, boolean), \
         SCHEMA_V2_PROP(category, string), \
         SCHEMA_V2_PROP(groupSubDevices, boolean) \
    )

#define JSON_SCHEMA_IO_PERFORMANCE_VALIDATE_DRIVE_NAME \
     SCHEMA_V2_3( \
         ",\"required\":[\"driveName\"]" , \
          SCHEMA_V2_PROP(driveName, string), \
          SCHEMA_V2_PROP(mountName, string), \
          SCHEMA_V2_PROP(chunkSize, integer) \
    )

#define JSON_SCHEMA_MOUNT_AND_FULL_FSCK_VALIDATE_MOUNT_NAME \
     SCHEMA_V2_2( \
         ",\"required\":[\"mountName\"]" , \
          SCHEMA_V2_PROP(mountName, string), \
          SCHEMA_V2_PROP(needFsck, boolean) \
    )
#endif //_SCHEMAVALIDATIONAPI_H
