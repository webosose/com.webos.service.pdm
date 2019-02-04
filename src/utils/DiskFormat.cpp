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

#include "DiskFormat.h"
#include "Common.h"
#include "PdmLogUtils.h"
#include <cstdlib>

DiskFormat::DiskFormat()
{

}

DiskFormat::~DiskFormat()
{
}


PdmDevStatus DiskFormat::formatDrive(const std::string driveName,const std::string fsType,const std::string &volumeLabel)
 {
    std::unordered_map<std::string,std::string>::iterator it;
    it = formatFsCommands.find(fsType);
    if (it == formatFsCommands.end()) {
        PDM_LOG_ERROR("DiskFormat:%s line: %d error on getFsType", __FUNCTION__, __LINE__);
        return PdmDevStatus::PDM_DEV_UNSUPPORTED_FS;
    }

    std::string syscommand(formatFsCommands[fsType]);

    if(!volumeLabel.empty())
    {
        syscommand += " /dev/"+ driveName + volumeLabelOptions[fsType] + volumeLabel;
    }
    else
        syscommand += " /dev/"+ driveName;

    PDM_LOG_INFO("DiskFormat:",0,"%s line: %d system command: %s", __FUNCTION__,__LINE__,syscommand.c_str());
    int ret  = system(syscommand.c_str());
    if (ret){
        PDM_LOG_ERROR("DiskFormat:%s line: %d Format failed", __FUNCTION__, __LINE__);
        return PdmDevStatus::PDM_DEV_FORMAT_FAIL;
    }
    return PdmDevStatus::PDM_DEV_SUCCESS;
 }
