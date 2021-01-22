// Copyright (c) 2019-2021 LG Electronics, Inc.
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

#include <experimental/filesystem>

#include "Common.h"
#include "DiskFormat.h"
#include "PdmFs.h"
#include "PdmFsck.h"
#include "PdmLogUtils.h"
#include "PdmUtils.h"

#include <unordered_map>
extern "C" {
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <mntent.h>
}

namespace pdmfileSys = std::experimental::filesystem;

using namespace PdmDevAttributes;

std::unordered_map<std::string,std::string> mountData = {
        { PDM_DRV_TYPE_NOFS,  ""},
        { PDM_DRV_TYPE_NTFS,  "uid=0,gid=5000,umask=0002"},
        { PDM_DRV_TYPE_TNTFS, "nls=utf8,max_prealloc_size=64m,uid=0,gid=5000,umask=0002"},
        { PDM_DRV_TYPE_FAT,   "shortname=mixed,uid=0,gid=5000,umask=0002"},
        { PDM_DRV_TYPE_TFAT,  "iocharset=utf8,fastmount=1,max_prealloc_size=32m,uid=0,gid=5000,umask=0002"},
        { PDM_DRV_TYPE_JFS,   "" },
        { PDM_DRV_TYPE_EXT2,  "" },
        { PDM_DRV_TYPE_EXT3,  "data=journal"},
        { PDM_DRV_TYPE_EXT4,  "" },
        { PDM_DRV_TYPE_ERR,   ""},
    };
PdmDevStatus PdmFs::format(DiskPartitionInfo *partition, std::string &fileSysType,const std::string &label)
{
    if(!isSupportedFileSystem(fileSysType, sMapStorageType[partition->getStorageType()])){
        PDM_LOG_WARNING("PdmFs:%s line: %d Unsupported File system ", __FUNCTION__, __LINE__);
        setDefaultFsForFormat(partition, fileSysType);
    }

    DiskFormat dformatUtil;
    PdmDevStatus result = dformatUtil.formatDrive(partition->getDriveName(),fileSysType,label);
    if (result == PdmDevStatus::PDM_DEV_SUCCESS) {
        partition->setFsType(fileSysType);
        partition->setVolumeLabel(label);
    }
    return result;
}

void PdmFs::setDefaultFsForFormat(DiskPartitionInfo *partition, std::string &fileSysType)
{
    if(partition->getStorageTypeString() == PdmDevAttributes::PDM_STORAGE_USB_FLASH)
        fileSysType = PDM_DRV_TYPE_FAT;

    else if(partition->getStorageTypeString() == PdmDevAttributes::PDM_STORAGE_USB_HDD)
        fileSysType = PDM_DRV_TYPE_NTFS;

    else if(partition->getStorageTypeString() == PdmDevAttributes::PDM_STORAGE_EMMC)
        fileSysType = PdmDevAttributes::PDM_DRV_TYPE_EXT4;
    else
    {
        if(partition->getFsType().empty())
            fileSysType = PDM_DRV_TYPE_FAT;
        else
            fileSysType = partition->getFsType();
    }
}

PdmDevStatus PdmFs::fsck(DiskPartitionInfo &partition, const std::string &fsckMode)
{
    PDM_LOG_INFO("PdmFs:",0,"%s line: %d driveType : %s partitionName : %s",
                 __FUNCTION__,__LINE__,partition.getFsType().c_str(), partition.getDriveName().c_str());
    PdmFsck dfsckObj;
    partition.setFsckStatus(PARTITION_FSCK_STARTED);
    PdmDevStatus fsckStatus =  dfsckObj.fsck(fsckMode, partition.getFsType(), partition.getDriveName());
    partition.setFsckStatus(fsckStatus);
    if(PdmDevStatus::PDM_DEV_FSCK_TIMEOUT == fsckStatus)
        partition.setDriveStatus(PDM_ERR_NEED_FSCK);
    return fsckStatus;
}

bool PdmFs::mountPartition(DiskPartitionInfo &partition, const bool &readOnly)
{
    if(!pdmfileSys::is_directory(partition.getMountName()))
        return false;

    std::string fsType = partition.getFsType();
    const char *data   = mountData[fsType].c_str();
    uint64_t mountFlag = mountflags(fsType, readOnly);
    std::string driveName("/dev/");

    driveName.append(partition.getDriveName());

    int32_t ret = mount(driveName.c_str(), partition.getMountName().c_str() ,fsType.c_str(),
                        mountFlag,(void*)data);

    if(ret){
        PDM_LOG_WARNING("PdmFs:%s line: %d Partition %s mount failed retry with readonly. errno: %d strerror: %s", __FUNCTION__, __LINE__,driveName.c_str(),errno, strerror(errno));
        if( (errno == EACCES) || (errno == EROFS) ) {
            mountFlag |= MS_RDONLY;
            ret = mount(driveName.c_str(), partition.getMountName().c_str() ,fsType.c_str(),mountFlag,(void*)data);
        }
    }

    if(!ret) {
        SpaceInfo spaceData = {0};
        if(calculateSpaceInfo(partition.getMountName(), &spaceData)) {
            partition.setDriveSize(spaceData.driveSize);
            partition.setUsedSize(spaceData.usedSize);
            partition.setFreeSize(spaceData.freeSize);
            partition.setUsedRate(spaceData.usedRate);
        }
        PDM_LOG_DEBUG("PdmFs:%s line: %d Partition %s is mounted. ", __FUNCTION__, __LINE__,partition.getMountName().c_str());
        return true;
    }

    PDM_LOG_ERROR("PdmFs:%s line: %d  %s mount Failed", __FUNCTION__, __LINE__,partition.getMountName().c_str());
    return false;
}

bool PdmFs::umount(DiskPartitionInfo &partition, const bool lazyUnmount) const
{
    bool retValue = true;
    int umountFlags = MNT_FORCE;

    if(lazyUnmount)
        umountFlags |= MNT_DETACH;

    int res = umount2(partition.getMountName().c_str(), umountFlags);
    if(res != 0) {
        PDM_LOG_ERROR("PdmFs:%s line: %d Umount Failed umountALLPartition", __FUNCTION__, __LINE__);
        retValue = false;
   }
   return retValue;
}

PdmDevStatus PdmFs::setVolumeLabel(DiskPartitionInfo *partition, const std::string &volLabel)
{
    int ret = -1;

    const std::string driveName = partition->getDriveName();
    partition->partitionLock();
    PDM_LOG_DEBUG("PdmFs:%s line: %d Volume label to be set : %s", __FUNCTION__, __LINE__, volLabel.c_str());
    if ( volLabel.length() <= 0 ) {
        PDM_LOG_WARNING("PdmFs:%s line: %d Volume label is empty", __FUNCTION__, __LINE__);
        partition->partitionUnLock();
        return PdmDevStatus::PDM_DEV_VOLUME_LABEL_EMPTY;
    }
    std::string sysCommand = "";
    std::string fsType = partition->getFsType();

     PDM_LOG_DEBUG("PdmFs:%s line: %d File system type : %s", __FUNCTION__, __LINE__, fsType.c_str());

    if ( fsType == "tntfs" || fsType == "ntfs") {
        sysCommand = "ntfslabel -f  /dev/" + driveName + " " + volLabel;
    } else if ( fsType == "vfat" || fsType == "tfat" ) {
        sysCommand = "fatlabel -f -l "+ volLabel + " /dev/"+ driveName;
    } else if ( fsType == "ext2" || fsType == "ext3" || fsType == "ext4" ) {
        sysCommand = "e2label  /dev/" + driveName + " " + volLabel;
    } else {
        partition->partitionUnLock();
        PDM_LOG_WARNING("PdmFs:%s line: %d Unsupporetd File system", __FUNCTION__, __LINE__);
        return PdmDevStatus::PDM_DEV_UNSUPPORTED_FS;
    }
    PDM_LOG_INFO("PdmFs:",0,"%s line: %d System command to set label : %s", __FUNCTION__,__LINE__,sysCommand.c_str());

    ret = system(sysCommand.c_str());

    if (ret == -1 || ret == 127 ) {
        PDM_LOG_ERROR("PdmFs:%s line: %d Setting volume Label:%s failed", __FUNCTION__, __LINE__, volLabel.c_str());
        partition->partitionUnLock();
        return PdmDevStatus::PDM_DEV_SET_VOLUME_LABEL_FAIL;
    } else {
        partition->setVolumeLabel(volLabel);
        PDM_LOG_DEBUG("PdmFs:%s line: %d Setting volume Label:%s success", __FUNCTION__, __LINE__, volLabel.c_str());
        partition->partitionUnLock();
        return PdmDevStatus::PDM_DEV_SUCCESS;
    }
}
PdmDevStatus PdmFs::isWritable(DiskPartitionInfo *partition,bool &isWritable)
{
    bool result = false;
    if(isDriveBusy(*partition))
        return PdmDevStatus::PDM_DEV_BUSY;

    FILE            *pfile;
    std::string devName = "/dev/" + partition->getDriveName();

    if ((pfile = setmntent("/etc/mtab", "r")) != NULL)
    {
        struct mntent   *pmnt;
        while ((pmnt = getmntent(pfile)) != NULL)
        {
            if (strcmp(devName.c_str(), pmnt->mnt_fsname) == 0){
                if (hasmntopt(pmnt, "ro") == NULL)
                {
                    isWritable = true;
                    break;
                }
            }
        }
        endmntent (pfile);
    }

    PDM_LOG_DEBUG("PdmFs:%s line: %d drivename: %s is writable : %d", __FUNCTION__, __LINE__, devName.c_str(),result);
    return PdmDevStatus::PDM_DEV_SUCCESS;
}

bool PdmFs::isSupportedFileSystem(const std::string fsType, const std::string &storageType)
{
    if(fsType.empty())
        return false;

    if (storageType == PdmDevAttributes::PDM_STORAGE_USB_UNDEFINED || storageType == PdmDevAttributes::PDM_STORAGE_STREAMING)
        return false;

    if (fsType == PdmDevAttributes::PDM_DRV_TYPE_FAT  ||
        fsType == PdmDevAttributes::PDM_DRV_TYPE_NTFS ||
        fsType == PdmDevAttributes::PDM_DRV_TYPE_EXT2 ||
        fsType == PdmDevAttributes::PDM_DRV_TYPE_EXT3 ||
        fsType == PdmDevAttributes::PDM_DRV_TYPE_EXT4)
            return true;
    return false;
}

bool PdmFs::calculateSpaceInfo(const std::string &mountName, SpaceInfo *spaceInfo) {

    struct statfs fsInfo = {0};
    if (statfs( mountName.c_str(), &fsInfo ) != 0) {
        PDM_LOG_ERROR("PdmFs:%s line: %d statfs failed for mountName:%s ", __FUNCTION__, __LINE__, mountName.c_str());
        return false;
    }

    spaceInfo->driveSize = ( fsInfo.f_blocks * (fsInfo.f_bsize / 1024) );
    spaceInfo->freeSize  = ( fsInfo.f_bavail * (fsInfo.f_bsize / 1024) );

     if (spaceInfo->driveSize > spaceInfo->freeSize)
         spaceInfo->usedSize = spaceInfo->driveSize - spaceInfo->freeSize;
     if (spaceInfo->driveSize)
         spaceInfo->usedRate = spaceInfo->usedSize * 100 / spaceInfo->driveSize;
    PDM_LOG_DEBUG("PdmFs:%s line: %d DriveSize : %llu KBytes UsedSize: %llu KBytes FreeSize: %llu KBytes UsedRate: %llu", __FUNCTION__, __LINE__,spaceInfo->driveSize,spaceInfo->usedSize,spaceInfo->freeSize,spaceInfo->usedRate);
    return true;


}

uint64_t PdmFs::mountflags(const std::string &fsType, const bool &readOnly) {

    uint64_t mountFlag = MS_MGC_VAL;

    if(fsType == PDM_DRV_TYPE_EXT3 || fsType == PDM_DRV_TYPE_EXT4 || fsType == PDM_DRV_TYPE_JFS)
        mountFlag |= (MS_NOATIME | MS_NODIRATIME);

    else if(fsType == PDM_DRV_TYPE_FAT)
        mountFlag |= MS_RELATIME;

    if(readOnly)
        mountFlag |= MS_RDONLY;

    return mountFlag;
}


void PdmFs::checkFileSystem(DiskPartitionInfo &partition) {

    bool result = isSupportedFileSystem(partition.getFsType(), partition.getStorageTypeString());
    partition.setIsSupportedFs(result);
    bool isDirCreated =  PdmUtils::createDir(partition.getMountName());
    PDM_LOG_INFO("DiskPartitionInfo:",0,"%s line: %d Directory is created? %s Mount path: %s",
                            __FUNCTION__,__LINE__,isDirCreated?"YES":"NO", partition.getMountName().c_str());
}

bool PdmFs::isDriveBusy(DiskPartitionInfo &partition) const
{
    PDM_LOG_INFO("PdmFs:",0,"%s line: %d Drive Mount Name: %s", __FUNCTION__,__LINE__,partition.getMountName().c_str());
    std::string sysCommand("lsof +D ");
    sysCommand.append(partition.getMountName());
    std::string result = PdmUtils::execShellCmd(sysCommand);
    if(!result.empty()){
        PDM_LOG_ERROR("PdmFs:%s line: %d error on isDriveBusy", __FUNCTION__, __LINE__);
        return true;
    }

    return false;
}
