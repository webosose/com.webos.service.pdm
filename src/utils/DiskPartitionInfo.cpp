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

#include "Common.h"
#include "DiskPartitionInfo.h"
#include "PdmLogUtils.h"
#include "PdmNetlinkEvent.h"

using namespace PdmDevAttributes;

DiskPartitionInfo::DiskPartitionInfo(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
            : Storage(pConfObj, pluginAdapter,"USB_STORAGE",PDM_ERR_NOMOUNTED,StorageInterfaceTypes::USB_UNDEFINED)
            , m_isMounted(false)
            , m_fsckStatus(PARTITION_FSCK_NONE)
            , m_isSupportedFS(false)
            , driveStatus(MOUNT_NOT_OK)
{
}

void DiskPartitionInfo::setPartitionInfo(PdmNetlinkEvent* pNE,const std::string &deviceRootPath)
{
    driveName = pNE->getDevAttribute(DEVNAME);
    PDM_LOG_INFO("DiskPartitionInfo:",0,"%s driveName: %s deviceRootPath:%s", __FUNCTION__,driveName.c_str(), deviceRootPath.c_str());
    if(!driveName.empty() && !deviceRootPath.empty())
    {
#ifndef WEBOS_SESSION
        rootPath = deviceRootPath;
        mountName = deviceRootPath + "/" + driveName;
#endif
    }
    if(!pNE->getDevAttribute(ID_FS_TYPE).empty())
        fsType = pNE->getDevAttribute(ID_FS_TYPE);

    if(!pNE->getDevAttribute(ID_FS_UUID).empty())
        uuid = pNE->getDevAttribute(ID_FS_UUID);
    if(!pNE->getDevAttribute(ID_FS_LABEL_ENC).empty())
        volumeLabel = pNE->getDevAttribute(ID_FS_LABEL_ENC);
}

void DiskPartitionInfo::setDriveStatus(std::string dStatus)
{
    m_isMounted = false;
    driveStatus = dStatus;
    if( driveStatus == MOUNT_OK )
        m_isMounted = true;
}

bool DiskPartitionInfo::partitionLock()
{
    return cmdExectionLock.try_lock();
}
void DiskPartitionInfo::partitionUnLock()
{
    cmdExectionLock.unlock();
}


void DiskPartitionInfo::setFsType(const std::string &type) {
    if(type.empty()) {
        PDM_LOG_ERROR("DiskFormat:%s line: %d file system type is empty", __FUNCTION__, __LINE__);
        return;
    }
    fsType  = type;
}

void DiskPartitionInfo::setVolumeLabel(const std::string &label) {
    if(label.empty()) {
        PDM_LOG_ERROR("DiskFormat:%s line: %d label is empty", __FUNCTION__, __LINE__);
        return;
    }
    volumeLabel = label;
}

