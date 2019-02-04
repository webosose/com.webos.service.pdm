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

#include "PdmFsck.h"
#include "StorageDevice.h"
#include "PdmLogUtils.h"
#include "Common.h"

using namespace PdmDevAttributes;

    fsckCmdMap fsckDriveTypeBinOptionMap =    {
        { PDM_DRV_TYPE_NOFS, { "", "" }},
        { PDM_DRV_TYPE_NTFS, { "ntfsck", "-p" }},
        { PDM_DRV_TYPE_TNTFS, { "ntfsck", "-p" }},
        { PDM_DRV_TYPE_FAT, { "fsck.vfat", "-a" }},
        { PDM_DRV_TYPE_TFAT, { "fsck.fat", "" }},
        { PDM_DRV_TYPE_JFS, { "jfs_fsck", "-Q -z 0x00000000" }},
        { PDM_DRV_TYPE_EXT2, { "fsck.ext2", "" }},
        { PDM_DRV_TYPE_EXT3, { "fsck.ext3", "" }},
        { PDM_DRV_TYPE_EXT4, { "fsck.ext4", "" }},
        { PDM_DRV_TYPE_FUSE_PTP, { "", "" }},
        { PDM_DRV_TYPE_FUSE_MTP, { "", "" }},
        { PDM_DRV_TYPE_EXFAT, { "", "" }},
        { PDM_DRV_TYPE_ERR, { "", "" }},
    };


PdmFsck::PdmFsck()
{

}
PdmFsck::~PdmFsck()
{

}

PdmDevStatus PdmFsck::fsck(const std::string& fsckMode,const std::string driveType,const std::string partitionName)
{
   PDM_LOG_DEBUG("PdmFsck: %s line: %d partitionName: %s, driveType: %s, fsckMode = %s", __FUNCTION__, __LINE__, partitionName.c_str(), driveType.c_str(),fsckMode.c_str());

    fsckBinOptionPair binOption = fsckDriveTypeBinOptionMap[driveType];
    std::string fsckbin = binOption.first;
     PDM_LOG_DEBUG("PdmFsck: %s line: %d fsckbin: %s ",__FUNCTION__,__LINE__,fsckbin.c_str());

    if(fsckbin.empty())
    {
        PDM_LOG_ERROR("PdmFsck: %s  line: %d partitionName: %s is not avalible to FSCK!!\n",__FUNCTION__,__LINE__,partitionName.c_str());
        return PdmDevStatus::PDM_DEV_FSCK_FAIL;
    }
    std::string fsckOption = binOption.second;
    std::string Mode = getFsckMode(driveType,fsckMode);
    std::string sysCommand;
    if(fsckMode == PDM_FSCK_AUTO && driveType != PDM_DRV_TYPE_TNTFS && driveType != PDM_DRV_TYPE_TFAT){
            sysCommand = timeout + fsckbin + " " + Mode + " " + fsckOption + " /dev/" + partitionName;
    }
    else
        sysCommand = fsckbin + " " + Mode + " " + fsckOption + " /dev/" + partitionName;

    PDM_LOG_INFO("PdmFsck:",0,"%s line: %d] %s FSCK Mode, %s FSCK fsckOption, %s FSCK sysCommand ", __FUNCTION__,__LINE__,Mode, fsckOption.c_str(), sysCommand.c_str());
    int32_t result = system(sysCommand.c_str());
    if (result == 1 || result == 2)
    {
        PDM_LOG_ERROR("PdmFsck: %s line: %d partitionName:%s FSCK fail !!\n",__FUNCTION__,__LINE__,partitionName.c_str());
        return PdmDevStatus::PDM_DEV_FSCK_FAIL;
    }
    else if(result == FSCK_COMMAND_TIMEOUT)
    {
        PDM_LOG_WARNING("PdmFsck: %s line: %d partitionName: %s FSCK Commands timeout !!\n",__FUNCTION__,__LINE__,partitionName.c_str());
        return PdmDevStatus::PDM_DEV_FSCK_TIMEOUT;
    }
    else
    {
        PDM_LOG_DEBUG("PdmFsck: %s line : %d  partitionName :%s FSCK success , result : %d!!\n",__FUNCTION__,__LINE__,partitionName.c_str(), result);
        return PdmDevStatus::PDM_DEV_SUCCESS;
    }
}

std::string PdmFsck::getFsckMode(std::string driveType, std::string fsckMode)
{
    std::string err;
    if (driveType.empty())
    {
        PDM_LOG_CRITICAL("PdmFsck: %s line: %d param error\n",__FUNCTION__,__LINE__);
        return err;
    }
    if (driveType == PDM_DRV_TYPE_TNTFS)
    {
        if (fsckMode.empty())    return "-C";
        else if (fsckMode == PDM_FSCK_AUTO) return "-C -at 20";
        else if (fsckMode == PDM_FSCK_FORCE) return "-C";
        else return "-C";
    }
    if (driveType == PDM_DRV_TYPE_TFAT)
    {

        if (fsckMode == PDM_FSCK_AUTO) return "-a -t 20";
        else return err;
    }
    else if (driveType == PDM_DRV_TYPE_JFS)
    {
        if (fsckMode.empty())    return "-a";
        else if (fsckMode == PDM_FSCK_AUTO) return "-a";
        else if (fsckMode == PDM_FSCK_FORCE) return "-f --omit_journal_replay";
        else return "-a";
    }
    else if (driveType == PDM_DRV_TYPE_EXT2 || driveType == PDM_DRV_TYPE_EXT3 || driveType == PDM_DRV_TYPE_EXT4)
    {
        if (fsckMode.empty())    return "-y";
        else if (fsckMode == PDM_FSCK_AUTO) return "-y";
        else if (fsckMode == PDM_FSCK_FORCE) return "-yf";
        else return "-y";
    }
    else
        return err;
}
