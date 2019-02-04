// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef _PDM_ERRORS_H
#define _PDM_ERRORS_H

#include <string>
#include <map>

namespace PdmErrors
{
    enum PdmResponseErrors {
        PDM_RESPONSE_FAILURE = -1,
        PDM_RESPONSE_SPACEINFO_FAILURE = 103,
        PDM_RESPOSE_CATEGORY_MISMATCH = 104
    };

    enum PdmDeviceErrors {
        PDM_DEV_SUCCESS = 1000,
        PDM_DEV_MOUNT_FAIL,
        PDM_DEV_MOUNT_ALL_FAIL,
        PDM_DEV_UMOUNT_FAIL,
        PDM_DEV_UMOUNT_ALL_FAIL,
        PDM_DEV_EJECT_FAIL,
        PDM_DEV_FORMAT_FAIL,
        PDM_DEV_FSCK_FAIL,
        PDM_DEV_IO_PERF_FAIL,
        PDM_DEV_FSCK_TIMEOUT,
        PDM_DEV_SET_VOLUME_LABEL_FAIL,
        PDM_DEV_VOLUME_LABEL_EMPTY,
        PDM_DEV_UNSUPPORTED_FS,
        PDM_DEV_BUSY,
        PDM_DEV_NOT_BUSY,
        PDM_DEV_IS_WRITABLE,
        PDM_DEV_READ_ONLY,
        PDM_DEV_DRIVE_NAME_EMPTY,
        PDM_DEV_PARTITION_NOT_FOUND,
        PDM_DEV_ROOT_PATH_EMPTY,
        PDM_DEV_DEVICE_NOT_FOUND,
        PDM_DEV_DRIVE_NOT_FOUND,
        PDM_DEV_DRIVE_NOT_MOUNTED,
        PDM_DEV_ERROR
    };

    enum PdmConfigErrors {
        PDM_CONFIG_ERROR_NONE = 2000,
        PDM_CONFIG_ERROR_LOAD,
        PDM_CONFIG_ERROR_NOCATEGORY,
        PDM_CONFIG_ERROR_NOKEY
    };

    static std::map<int, std::string> mPdmErrorTextTable =
    {
        {PDM_RESPONSE_FAILURE,                      "Failure"},
        {PDM_RESPONSE_SPACEINFO_FAILURE,            "Get Space Info fail"},
        {PDM_DEV_SUCCESS,                           "Device operation successful"},
        {PDM_DEV_MOUNT_FAIL,                        "Failed to mount drive"},
        {PDM_DEV_MOUNT_ALL_FAIL,                    "Failed to mount all drives"},
        {PDM_DEV_UMOUNT_FAIL,                       "Failed to un-mount drive"},
        {PDM_DEV_UMOUNT_ALL_FAIL,                   "Failed to un-mount all drives"},
        {PDM_DEV_EJECT_FAIL,                        "Failed to eject device"},
        {PDM_DEV_FORMAT_FAIL,                       "Failed to format drive"},
        {PDM_DEV_FSCK_TIMEOUT,                      "Fsck command has timed out"},
        {PDM_DEV_FSCK_FAIL,                         "Failed to file system check for drive"},
        {PDM_DEV_IO_PERF_FAIL,                      "Failed to test IO performance"},
        {PDM_DEV_SET_VOLUME_LABEL_FAIL,             "Failed to set volume label for drive"},
        {PDM_DEV_VOLUME_LABEL_EMPTY,                "Volume label is empty"},
        {PDM_DEV_UNSUPPORTED_FS,                    "Device has unsupported file system"},
        {PDM_DEV_BUSY,                              "Device is busy"},
        {PDM_DEV_NOT_BUSY,                          "Device is free"},
        {PDM_DEV_IS_WRITABLE,                       "Drive is writable"},
        {PDM_DEV_READ_ONLY,                         "Drive is read only"},
        {PDM_DEV_DRIVE_NAME_EMPTY,                  "Drive name is empty"},
        {PDM_DEV_PARTITION_NOT_FOUND,               "Partition not found in device"},
        {PDM_DEV_ROOT_PATH_EMPTY,                   "Device root path is empty"},
        {PDM_DEV_DEVICE_NOT_FOUND,                  "Device not found"},
        {PDM_DEV_DRIVE_NOT_FOUND,                   "Drive not found in device"},
        {PDM_DEV_DRIVE_NOT_MOUNTED,                 "Drive not mounted"},
        {PDM_DEV_ERROR,                             "Device error unknown"},
        {PDM_CONFIG_ERROR_NONE,                     "Configuration success"},
        {PDM_CONFIG_ERROR_LOAD,                     "Failed to load configuration"},
        {PDM_CONFIG_ERROR_NOCATEGORY,               "Category not found in config"},
        {PDM_CONFIG_ERROR_NOKEY,                    "Key not found in config"},
        {PDM_RESPOSE_CATEGORY_MISMATCH,             "Category is not matching"}
    };
    void logPdmErrorCodeAndText(int errorCode);
};

using PdmDevStatus = PdmErrors::PdmDeviceErrors;
using PdmConfigStatus = PdmErrors::PdmConfigErrors;
using PdmPayload = PdmErrors::PdmResponseErrors;

#endif //_PDM_ERRORS_H
