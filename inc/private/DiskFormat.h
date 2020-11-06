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

#ifndef DISKFORMAT_H_
#define DISKFORMAT_H_

#include <string>
#include <array>
#include <unordered_map>
#include "PdmErrors.h"
#include "Common.h"

const int FORMATCMDSIZE = 9;
const int FSTYPELISTSIZE = 13;
const int FSTYPEERROR = -1;

class DiskFormat {

std::unordered_map<std::string,std::string> volumeLabelOptions = {
    {PdmDevAttributes::PDM_DRV_TYPE_NOFS,    ""},
    {PdmDevAttributes::PDM_DRV_TYPE_NTFS,    " -L "},
    {PdmDevAttributes::PDM_DRV_TYPE_FAT,     " -n "},
    {PdmDevAttributes::PDM_DRV_TYPE_JFS,     " -L "},
    {PdmDevAttributes::PDM_DRV_TYPE_EXT2,    " -L "},
    {PdmDevAttributes::PDM_DRV_TYPE_EXT3,    " -L "},
    {PdmDevAttributes::PDM_DRV_TYPE_EXT4,    " -L "},
};

std::unordered_map<std::string, std::string> formatFsCommands = {
    {PdmDevAttributes::PDM_DRV_TYPE_NOFS,    ""},
    {PdmDevAttributes::PDM_DRV_TYPE_NTFS,    "mkntfs -F -Q "},
#ifdef WEBOS_SESSION
    {PdmDevAttributes::PDM_DRV_TYPE_FAT,     "mkfs.vfat " },
#else
    {PdmDevAttributes::PDM_DRV_TYPE_FAT,     "mkfs.vfat -F 32 " },
#endif
    {PdmDevAttributes::PDM_DRV_TYPE_JFS,     "jfs_mkfs -q -z 0x00000000 "},
    {PdmDevAttributes::PDM_DRV_TYPE_EXT2,    "mkfs.ext2 -F "},
    {PdmDevAttributes::PDM_DRV_TYPE_EXT3,    "mkfs.ext3 -F "},
    {PdmDevAttributes::PDM_DRV_TYPE_EXT4,    "mkfs.ext4 -F "},
};

public:
   DiskFormat();
   ~DiskFormat();

   PdmDevStatus formatDrive(const std::string driveName,const std::string fsType,const std::string &volumeLabel);
};

#endif //DISKFORMAT_H_
