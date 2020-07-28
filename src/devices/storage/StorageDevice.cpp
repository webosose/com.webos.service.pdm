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

#include <errno.h>
#include <memory>
#include <string.h>
#include "StorageDevice.h"
#include "PdmLogUtils.h"
#include "PdmSmartInfo.h"
#include "PdmUtils.h"
#include "StorageDeviceHandler.h"

#define PDM_HDD_ID_ATA "1"
#define PDM_HARD_DISK "HDD"
#define PDM_STORAGE_DEVICE_CONNECTION_TIME 10000
#define USB30_BLACKDEVICE "USB30_BLACKDEVICE"

using namespace PdmDevAttributes;
using namespace PdmErrors;

StorageDevice::StorageDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
            : Storage(pConfObj, pluginAdapter,"USB_STORAGE",PDM_ERR_NOMOUNTED,StorageInterfaceTypes::USB_UNDEFINED)
            , m_deviceIsMounted(false)
            , m_hasUnsupportedFs(false)
            , m_isDevAddEventNotified(false)
            , m_isSdCardRemoved (false)
            , m_isExtraSdCard(false)
            , m_partitionCount(0)
            , m_timeoutId(0)
            , m_fsckThreadCount(0)
{
    mHddDiskStats = std::make_tuple(-1,-1,-1);
}

StorageDevice::~StorageDevice() {

    try {
        deletePartitionData();
    }
    catch (std::exception &e) {
        PDM_LOG_ERROR("StorageDevice:%s line: %d Caught exception: %s", __FUNCTION__, __LINE__, e.what());
    }
}

/*
 countPartitions
 @return int
 To get number of partitions available in device
*/
int StorageDevice::countPartitions(const std::string &devName)
{
    int count = 0;
    if(devName.empty())
        return count;
    std::string sysCommand = "lsblk -n /dev/" + devName + " | grep -c part";
    std::string partitions = PdmUtils::execShellCmd(sysCommand);
    count = std::stoi(partitions);
    return count;
}

void StorageDevice::setStorageInterfaceType(PdmNetlinkEvent* pNE)
{
    if(pNE->getDevAttribute(CARD_READER) == YES)
        setStorageType(StorageInterfaceTypes::USB_CARD_READER);
    else if(pNE->getDevAttribute(HARD_DISK) == YES)
        setStorageType(StorageInterfaceTypes::USB_HDD);
    else
        setStorageType(StorageInterfaceTypes::USB_STICK);
}


/*
 setDeviceInfo
 @return
 Set the device info on device detections
*/
void StorageDevice::setDeviceInfo(PdmNetlinkEvent* pNE)
{
    if(triggerUevent()) {
        PDM_LOG_DEBUG("StorageDevice:%s line: %d ACTION: %s uevent triggered", __FUNCTION__, __LINE__,pNE->getDevAttribute(ACTION).c_str());
        return;
    }

    PDM_LOG_DEBUG("StorageDevice:%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVTYPE).c_str(),pNE->getDevAttribute(ACTION).c_str());
    switch(sMapUsbDeviceType[pNE->getDevAttribute(DEVTYPE)])
    {
        case UsbDeviceTypes::TYPE_DEV_USB:
            updateDeviceInfo(pNE);
            break;
        case UsbDeviceTypes::TYPE_DEV_DISK:
            PDM_LOG_DEBUG("StorageDevice:%s line: %d DEVTYPE: %s ACTION: %s m_storageType: %d", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVTYPE).c_str(),pNE->getDevAttribute(ACTION).c_str(), m_storageType);
            updateDiskInfo(pNE);
            updateMultiSdCard(pNE);
            checkSdCardAddRemove(pNE);
            break;
        case UsbDeviceTypes::TYPE_DEV_PARTITION:
            setPartitionInfo(pNE);
            break;
         default:
             //DO nothing
             break;
    }
}

void StorageDevice::updateMultiSdCard(PdmNetlinkEvent* pNE) {
    PDM_LOG_DEBUG("StorageDevice:%s line: %d NAME : %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVNAME).c_str());
    if(m_devicePath.empty()) {
        PDM_LOG_DEBUG("StorageDevice:%s line: %d updated NAME: %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVNAME).c_str());
        m_devicePath = pNE->getDevAttribute(DEVPATH);
        m_serialNumber = pNE->getDevAttribute(ID_SERIAL_SHORT);
        m_deviceSubType = pNE->getDevAttribute(ID_USB_DRIVER);
        m_productName = pNE->getDevAttribute(ID_MODEL);
        if(!m_pluginAdapter->getPowerState() || pNE->getDevAttribute(IS_POWER_ON_CONNECT) == "true")
            m_isPowerOnConnect = true;
        if(!pNE->getDevAttribute(ID_VENDOR_FROM_DATABASE).empty()){
            m_vendorName = pNE->getDevAttribute(ID_VENDOR_FROM_DATABASE);
        } else {
            m_vendorName = pNE->getDevAttribute(ID_VENDOR);
        }
        if(!pNE->getDevAttribute(ID_INSTANCE).empty()) {
            std::string instance = pNE->getDevAttribute(ID_INSTANCE);
            std::size_t found = instance.find_first_of(":");
            int instanceNum = stoi(instance.substr(found+1));
            m_deviceNum += instanceNum;
        }
    }
}

void StorageDevice::updateDeviceInfo(PdmNetlinkEvent* pNE)
{
    PDM_LOG_DEBUG("StorageDevice:%s line: %d DEVNAME: %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVNUM).c_str());
    Device::setDeviceInfo(pNE);
    m_devSpeed = getDeviceSpeed(stoi(pNE->getDevAttribute(SPEED)));
    if((!pNE->getDevAttribute(ID_BLACK_LISTED_SUPER_SPEED_DEVICE).empty()) && (pNE->getDevAttribute(ID_BLACK_LISTED_SUPER_SPEED_DEVICE) == YES) )
    {
            PDM_LOG_INFO("StorageDevice:",0,"%s line: %d This is a black listed super speed device", __FUNCTION__,__LINE__);
            m_errorReason = USB30_BLACKDEVICE;
    }
}

void StorageDevice::updateDiskInfo(PdmNetlinkEvent* pNE)
{
    PDM_LOG_DEBUG("StorageDevice:%s line: %d ACTION = %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(ACTION).c_str());
    switch(sMapDeviceActions[pNE->getDevAttribute(ACTION)])
    {
        case DeviceActions::USB_DEV_ADD:
            m_deviceName = pNE->getDevAttribute(DEVNAME);
            if(pNE->getDevAttribute(READ_ONLY) == YES)
                isReadOnly = true; // disk is read only, FSCK will not be done

            setStorageInterfaceType(pNE);
            if(!rootPath.empty())
                rootPath.append((m_deviceName.substr(m_deviceName.find_last_of("/") + 1)));
            m_partitionCount = countPartitions(m_deviceName);
           PDM_LOG_DEBUG("StorageDevice:%s line: %d m_deviceName: %s rootPath: %s partition count: %d", __FUNCTION__, __LINE__,m_deviceName.c_str(),rootPath.c_str(), m_partitionCount);
            if(m_partitionCount == 0)
            {
                if(getStorageType() != StorageInterfaceTypes::USB_CARD_READER)
                {
                    PDM_LOG_DEBUG("StorageDevice:%s line: %d Has disk. But partition count is %d. Could be empty partition device. Set partition data now", __FUNCTION__, __LINE__, m_partitionCount);
                    setPartitionInfo(pNE);
                }
            }
            break;
        case DeviceActions::USB_DEV_CHANGE:
            if(m_storageType == StorageInterfaceTypes::USB_CARD_READER) {
                handleCardReaderDeviceChange(pNE);
                return;
            }
            if(countPartitions(m_deviceName) == 0 && pNE->getDevAttribute(CARD_READER)!= YES)
            {
                if(getStorageType() != StorageInterfaceTypes::USB_CARD_READER)
                {
                    PDM_LOG_DEBUG("StorageDevice:%s line: %d Has disk. But partition count is %d. Could be empty partition device. Set partition data now", __FUNCTION__, __LINE__, m_partitionCount);
                    setPartitionInfo(pNE);
                }
            }
            break;
        default:
            //Do nothing
            break;
    }
}

void StorageDevice::handleCardReaderDeviceChange(PdmNetlinkEvent* pNE)
{
    int tempPartitionCount = countPartitions(m_deviceName);
    PDM_LOG_DEBUG("StorageDevice:%s line: %d ACTION = CHANGE for %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVNAME).c_str());
    if(tempPartitionCount != m_partitionCount) {
        if(m_partitionCount!=0) {
            deletePartitionData();
            m_storageDeviceHandlerCb(REMOVE,nullptr);
        } else {
            m_partitionCount = tempPartitionCount;
        }
    }
}

/*
 setPartitionInfo
 @return
 Set the device info on device detections
 Called only if its an empty partition case where DEVTYPE is DISK
 and partition count is 0 or if DEVTYPE is USB_PARTITION
 */
void StorageDevice::setPartitionInfo(PdmNetlinkEvent* pNE)
{
    PDM_LOG_DEBUG("StorageDevice:%s line: %d Set the parition info %s", __FUNCTION__, __LINE__, m_deviceName.c_str());
       DiskPartitionInfo *partitionInfo = findPartition(pNE->getDevAttribute(DEVNAME));
    /* in some of the card reader add=>remove=>add action is occurring to handle this we need to check
    * partition before creating it*/
      if(partitionInfo)
           return;
       partitionInfo  = new (std::nothrow) DiskPartitionInfo(m_pConfObj, m_pluginAdapter);
       if(nullptr == partitionInfo) {
           PDM_LOG_CRITICAL("StorageDevice:%s line: %d Unable to create new DiskPartitionInfo", __FUNCTION__, __LINE__);
           return;
       }
    partitionInfo->setStorageType(m_storageType);
    partitionInfo->setProductName(getProductName());
    partitionInfo->setPartitionInfo(pNE, rootPath);
    m_pdmFileSystemObj.checkFileSystem(*partitionInfo);
    m_diskPartitionList.push_back(partitionInfo);
    if( m_partitionCount == 0 ||  (getdiskPartitionListSize() == getPartitionCount())){
        if(std::any_of(m_diskPartitionList.begin(), m_diskPartitionList.end(), [&](DiskPartitionInfo* dev){return dev->isSupportedFs();})){
            if(isReadOnly){
                mountAllPartition(); // direct mount without FSCK
                storageDeviceNotification();
            } else {

                for(auto partitionInfo : m_diskPartitionList){
                    if(partitionInfo->isSupportedFs()){
                        m_fsckThreadArray.push_back(std::thread(&StorageDevice::fsckOnDeviceAddThread, this, partitionInfo));
                        m_fsckThreadCount++;
                   }
                }
                m_timeoutId = g_timeout_add (PDM_STORAGE_DEVICE_CONNECTION_TIME,(GSourceFunc)notifyStorageConnecting, this);
                }
        }else{
            m_errorReason = PDM_ERR_UNSUPPORT_FS;
            m_hasUnsupportedFs = true;
            m_storageDeviceHandlerCb(ADD,this);
            m_isDevAddEventNotified = true;
            m_storageDeviceHandlerCb(UNSUPPORTED_FS_FORMAT_NEEDED,this);
        }
    }
}

void StorageDevice:: storageDeviceFsckNotification()
{
    m_fsckThreadCount--;
    if(m_fsckThreadCount == 0 ) {
        storageDeviceNotification();
        g_source_remove(m_timeoutId);
    }
}

void StorageDevice::pdmSmartDeviceInfoLogger()
{
    //TODO: Based on the debug level in the product this has to be called.
    std::unique_ptr<PdmSmartInfo> smartInfo(new PdmSmartInfo());
    if(smartInfo)
    {
        smartInfo->setDeviceName(m_deviceName);
        smartInfo->logPdmSmartInfo();
    }
}

void StorageDevice::storageDeviceNotification()
{
    if(std::any_of(m_diskPartitionList.begin(), m_diskPartitionList.end(), [&](DiskPartitionInfo* dev){return (dev->getFsckStatus() == PDM_DEV_FSCK_TIMEOUT);})) {
        m_errorReason = PDM_ERR_NEED_FSCK;
        m_storageDeviceHandlerCb(FSCK_TIMED_OUT,this);
        m_storageDeviceHandlerCb(ADD,this);
        m_isDevAddEventNotified = true;
    }
    else if(std::any_of(m_diskPartitionList.begin(), m_diskPartitionList.end(), [&](DiskPartitionInfo* dev){return dev->isMounted();})){

        m_errorReason = PDM_ERR_NOTHING;
        m_deviceIsMounted = true;
        m_storageDeviceHandlerCb(ADD,this);
        m_isDevAddEventNotified = true;
    }

    pdmSmartDeviceInfoLogger();
}

bool StorageDevice::notifyStorageConnecting(StorageDevice *ptr)
{
    ptr->m_storageDeviceHandlerCb(CONNECTING,nullptr);
    return false;
}

/*
 setPartitionVolumeLabel
 @return bool
 set volume label for selected partitions
*/
PdmDevStatus StorageDevice::setPartitionVolumeLabel(const std::string &drivename,const std::string &volumeLabel)
{
    DiskPartitionInfo* partition = findPartition(drivename);
    if( !partition )
        return PdmDevStatus::PDM_DEV_PARTITION_NOT_FOUND;
    return m_pdmFileSystemObj.setVolumeLabel(partition,volumeLabel);
}

/*
 isWritable
 @return bool
 check mounted file system is writable
*/
PdmDevStatus StorageDevice::isWritable(const std::string &driveName, bool &isWritable)
{
    DiskPartitionInfo* partition = findPartition(driveName);
    if( !partition )
        return PdmDevStatus::PDM_DEV_PARTITION_NOT_FOUND;;
    return m_pdmFileSystemObj.isWritable(partition, isWritable);
}

/*
 umountAllPartition
 @return bool
 umout all mounted partition in device
*/
PdmDevStatus StorageDevice::umountAllPartition(const bool lazyUnmount)
{
    PdmDevStatus result = PdmDevStatus::PDM_DEV_SUCCESS;
    PDM_LOG_DEBUG("StorageDevice:%s line: %d", __FUNCTION__, __LINE__);
    for(auto partition : m_diskPartitionList ){
        if(umountPartition(*partition, lazyUnmount) != PdmDevStatus::PDM_DEV_SUCCESS)
            result = PdmDevStatus::PDM_DEV_UMOUNT_ALL_FAIL;
    }
    return result;
}

/*
 mountAllPartition
 @return bool
 mount all file system supported partition in device
*/
PdmDevStatus StorageDevice::mountAllPartition()
{
    PdmDevStatus result = PdmDevStatus::PDM_DEV_SUCCESS;
   for(auto partition : m_diskPartitionList ){
        if(partition->isSupportedFs()){
            if(mountPartition(*partition, isReadOnly) != PdmDevStatus::PDM_DEV_SUCCESS)
                result = PdmDevStatus::PDM_DEV_MOUNT_ALL_FAIL;
        }
    }
    return result;
}
/*
 findPartition
 @return DiskPartitionInfo*
 check the partition is available with drivename
*/
DiskPartitionInfo* StorageDevice::findPartition(const std::string &drivename)
{
    if(drivename.empty())
        return nullptr;

    for(auto partition : m_diskPartitionList ){
        if(drivename == partition->getDriveName()){
            return partition;
        }
    }
    return nullptr;
}

/*
 deletePartitionData
 @return
 umount the mounted partition
 remove the directory
 delete the partition
*/
void StorageDevice::deletePartitionData()
{
    umountAllPartition(true);
    for(auto partition : m_diskPartitionList){
        delete partition;
    }
    m_diskPartitionList.clear();
    m_partitionCount = 0;
}
/*
 formatDiskStart
 @return bool
 Initialize the format setup for device
*/
PdmDevStatus StorageDevice::formatDiskStart(const std::string driveName,std::string fsType,const std::string &volumeLabel)
{
    DiskPartitionInfo* partition = findPartition(driveName);
    if( !partition )
        return PdmDevStatus::PDM_DEV_PARTITION_NOT_FOUND;

    if(isReadOnly)
        return PdmDevStatus::PDM_DEV_FORMAT_FAIL;

    if(umountPartition(*partition, false) != PdmDevStatus::PDM_DEV_SUCCESS)
        return PdmDevStatus::PDM_DEV_UMOUNT_FAIL;

    m_storageDeviceHandlerCb(FORMAT_STARTED,partition);

    if(m_pdmFileSystemObj.format(partition,fsType,volumeLabel) == PdmDevStatus::PDM_DEV_SUCCESS
                         && mountPartition(*partition, false) == PdmDevStatus::PDM_DEV_SUCCESS) {
        m_errorReason = PDM_ERR_NOTHING;
        m_storageDeviceHandlerCb(FORMAT_SUCCESS,partition);
        return PdmDevStatus::PDM_DEV_SUCCESS;
    }
    PDM_LOG_ERROR("StorageDevice: %s line: %d format failed, partition name: %s ", __FUNCTION__, __LINE__, driveName.c_str());
    m_storageDeviceHandlerCb(FORMAT_FAIL,partition);
    return PdmDevStatus::PDM_DEV_FORMAT_FAIL;
}

/*
 fsck
 @return bool
 1.Before FSCK it will find the partition in device
 2.umount the partition
 3.FSCK selected partition
 4.mount the partition
*/
PdmDevStatus StorageDevice::fsck(const std::string driveName)
{
    DiskPartitionInfo* partition = findPartition(driveName);
    if( !partition )
        return PdmDevStatus::PDM_DEV_PARTITION_NOT_FOUND;

    if(isReadOnly)
        return PdmDevStatus::PDM_DEV_FSCK_FAIL;

    if(umountPartition(*partition, false) != PdmDevStatus::PDM_DEV_SUCCESS)
        return PdmDevStatus::PDM_DEV_UMOUNT_FAIL;

    if(fsckPartition(*partition, PDM_FSCK_AUTO) == PdmDevStatus::PDM_DEV_SUCCESS)
        return PdmDevStatus::PDM_DEV_SUCCESS;

    return PdmDevStatus::PDM_DEV_FSCK_FAIL;
}
/*
 deviceRemoved
 @return
 Device physically remove from USB port this function is called
 Delete all partition list and removes created directories
*/
void StorageDevice::onDeviceRemove()
{
    for(auto& fsckThread : m_fsckThreadArray)
        fsckThread.join();
    m_fsckThreadArray.clear();
    m_fsckThreadCount = 0;
    deletePartitionData();
    removeRootDir();
    checkRemovalNotifications();
}
/*
 checkRemovalNotifications
 @return
 To check if certain notifications are to be sent upon device removal
*/
void StorageDevice::checkRemovalNotifications()
{
    // If device is inserted and removed before mounting
    //If device is inserted but takes time for Fsck and mount.
    PDM_LOG_INFO("StorageDevice:",0,"%s line: %d", __FUNCTION__,__LINE__);
    if(!m_isDevAddEventNotified)
        m_storageDeviceHandlerCb(REMOVE_BEFORE_MOUNT,this);
    else if(m_hasUnsupportedFs)
        m_storageDeviceHandlerCb(REMOVE_UNSUPPORTED_FS,this);
}
/*
 removeRootDir
 @return
 To remove the root directory </tmp/usb/<device name>
*/
void StorageDevice::removeRootDir()
{
    PDM_LOG_INFO("StorageDevice:",0,"%s line: %d removeRootDir: %s", __FUNCTION__,__LINE__,rootPath.c_str());
    if(!rootPath.empty())
    {
        PdmUtils::removeDirRecursive(rootPath);
        rootPath.clear();
    }
}
/*
 eject
 @return bool
 Ejects all mounted partition on successes true else false
*/
PdmDevStatus StorageDevice::eject()
{
    if(umountAllPartition(false) == PdmDevStatus::PDM_DEV_SUCCESS) {
        m_errorReason = m_deviceStatus = PDM_ERR_EJECTED;
        return PdmDevStatus::PDM_DEV_SUCCESS;
    }
    return PdmDevStatus::PDM_DEV_EJECT_FAIL;
}

void StorageDevice::registerCallback(handlerCb storageDeviceHandlerCb) {
    m_storageDeviceHandlerCb = storageDeviceHandlerCb;
}

PdmDevStatus StorageDevice::enforceFsckAndMount(bool needFsck)
{
     PDM_LOG_DEBUG("StorageDevice:%s line: %d Enforcing fsck and mount needFsck: %d", __FUNCTION__, __LINE__,needFsck);
     PdmDevStatus result = PdmDevStatus::PDM_DEV_SUCCESS;
     for(auto partition : m_diskPartitionList ) {
        if(needFsck == false && partition->isMounted() == false) {
            if(mountPartition(*partition,false) != PdmDevStatus::PDM_DEV_SUCCESS) {
                PDM_LOG_ERROR("StorageDevice:%s line: %d Enforcing fsck and mount failed", __FUNCTION__, __LINE__);
                result = PdmDevStatus::PDM_DEV_ERROR;
            }
        } else if(partition->getFsckStatus() == PdmDevStatus::PDM_DEV_FSCK_TIMEOUT ) {

            if(fsckPartition(*partition, PDM_FSCK_FORCE) != PdmDevStatus::PDM_DEV_SUCCESS) {
                PDM_LOG_ERROR("StorageDevice:%s line: %d Enforcing fsck and mount failed", __FUNCTION__, __LINE__);
                result = PdmDevStatus::PDM_DEV_ERROR;
            }
        }

     }
     if(result == PdmDevStatus::PDM_DEV_SUCCESS) {
        m_errorReason = PDM_ERR_NOTHING;
        m_deviceIsMounted = true;
     }
     return result;
}

void StorageDevice::fsckOnDeviceAddThread(DiskPartitionInfo *partition)
{
    fsckPartition(*partition, PDM_FSCK_AUTO);
    storageDeviceFsckNotification();

}

DiskPartitionInfo* StorageDevice::getSpaceInfo(const std::string driveName, bool directCheck)
{
    DiskPartitionInfo* partition = findPartition(driveName);
    if(partition && (partition->isMounted()) && directCheck) {
        SpaceInfo spaceData = {0};
        if(!m_pdmFileSystemObj.calculateSpaceInfo(partition->getMountName(), &spaceData))
            return partition;
        partition->partitionLock();
        partition->setDriveSize(spaceData.driveSize);
        partition->setUsedSize(spaceData.usedSize);
        partition->setFreeSize(spaceData.freeSize);
        partition->setUsedRate(spaceData.usedRate);
        partition->partitionUnLock();
    }
    return partition;
}

PdmDevStatus StorageDevice::umountPartition(DiskPartitionInfo &partition, const bool lazyUnmount) {

    PdmDevStatus umountStatus = PdmDevStatus::PDM_DEV_SUCCESS;

    if(partition.isMounted() == false)
        return umountStatus;
    //if lazy umount option is true don't check the drive busy condition
    if(lazyUnmount == false && m_pdmFileSystemObj.isDriveBusy(partition) == true)
        return PdmDevStatus::PDM_DEV_BUSY;
    partition.partitionLock();
    partition.setDriveStatus(IS_UNMOUNTING);
    m_storageDeviceHandlerCb(UMOUNT,nullptr);
    if(m_pdmFileSystemObj.umount(partition, lazyUnmount)) {
        partition.setDriveStatus(UMOUNT_OK);
        m_storageDeviceHandlerCb(UMOUNT,nullptr);
    }else{
        partition.setDriveStatus(UMOUNT_NOT_OK);
        m_storageDeviceHandlerCb(MOUNT,nullptr);
        umountStatus = PdmDevStatus::PDM_DEV_UMOUNT_FAIL;
    }
    partition.partitionUnLock();
    return umountStatus;
}

PdmDevStatus StorageDevice::mountPartition(DiskPartitionInfo &partition, const bool readOnly) {

    PdmDevStatus mountStatus = PdmDevStatus::PDM_DEV_SUCCESS;
    partition.setDriveStatus(IS_MOUNTING);
    m_storageDeviceHandlerCb(MOUNT,nullptr);

    if(m_pdmFileSystemObj.mountPartition(partition, readOnly)){
        partition.setDriveStatus(MOUNT_OK);
        m_storageDeviceHandlerCb(MOUNT,nullptr);
    } else {
        partition.setDriveStatus(MOUNT_NOT_OK);
        mountStatus = PdmDevStatus::PDM_DEV_UMOUNT_FAIL;
        m_storageDeviceHandlerCb(UMOUNT,nullptr);
    }
    return mountStatus;
}

PdmDevStatus StorageDevice::fsckPartition(DiskPartitionInfo &partition, const std::string &fsckMode) {

    PdmDevStatus fsckStatus = PdmDevStatus::PDM_DEV_SUCCESS;
    m_storageDeviceHandlerCb(FSCK_STARTED, nullptr);
    fsckStatus = m_pdmFileSystemObj.fsck(partition, fsckMode);
    if( fsckStatus == PdmDevStatus::PDM_DEV_SUCCESS)
        fsckStatus = mountPartition(partition,false);
    m_storageDeviceHandlerCb(FSCK_FINISHED, nullptr);
    return fsckStatus;
}

void StorageDevice::suspendRequest() {
    PDM_LOG_DEBUG("StorageDevice:%s line: %d", __FUNCTION__, __LINE__);
    for(auto partition : m_diskPartitionList )
        partition->setPowerStatus(false);
}

bool StorageDevice::suspendUmountAllPartitions(const bool lazyUnmount) {

    bool retValue = true;
    PDM_LOG_DEBUG("StorageDevice:%s line: %d", __FUNCTION__, __LINE__);
    for(auto partition : m_diskPartitionList ) {
        if(lazyUnmount == false && m_pdmFileSystemObj.isDriveBusy(*partition) == true)
            return false;
        if(m_pdmFileSystemObj.umount(*partition, lazyUnmount) == false)
            retValue = false;
    }
    return retValue;
}

void StorageDevice::resumeRequest(const int &eventType) {
    PDM_LOG_DEBUG("StorageDevice:%s line: %d", __FUNCTION__, __LINE__);
    for(auto partition : m_diskPartitionList )
        partition->setPowerStatus(true);
    m_isPowerOnConnect = true;
    //HDD need to be checked with eject for mounting
    if(m_storageType == StorageInterfaceTypes::USB_HDD && m_errorReason != PDM_ERR_EJECTED ) {
        PDM_LOG_DEBUG("StorageDevice:%s line: %d HDD device Error Reason: %s", __FUNCTION__, __LINE__, m_errorReason.c_str());
        return ;
    }
    mountAllPartition();
    if(std::any_of(m_diskPartitionList.begin(), m_diskPartitionList.end(), [&](DiskPartitionInfo* dev){return dev->isMounted();})){
        m_errorReason = PDM_ERR_NOTHING;
        m_deviceIsMounted = true;
    }
}
/*This condition need to check when cards reader is connected with SD card
 * Not getting the remove event in this case need to handle it, removing the partition data
 * and notifying it
 */
void StorageDevice::checkSdCardAddRemove(PdmNetlinkEvent* pNE) {

    PDM_LOG_INFO("StorageDevice:",0,"%s line: %d : m_isSdCardRemoved :  %d", __FUNCTION__,__LINE__,m_isSdCardRemoved);

    if(m_storageType != StorageInterfaceTypes::USB_STICK){
        PDM_LOG_INFO("StorageDevice:",0,"%s line: %d : not USB stick", __FUNCTION__,__LINE__);
        return;
    }

    if(pNE->getDevAttribute(DISK_MEDIA_CHANGE) != YES){
        PDM_LOG_INFO("StorageDevice:",0,"%s line: %d : DISK_MEDIA_CHANGE is not preset", __FUNCTION__,__LINE__);
        return;
    }

    if(pNE->getDevAttribute(CARD_READER)!= YES ) {
        PDM_LOG_INFO("StorageDevice:",0,"%s line: %d : CARD_READER not a card reader", __FUNCTION__,__LINE__);
        return;
    }

    int count = countPartitions(m_deviceName);
    if(count == 0 ) {
        deletePartitionData();
        for(auto partition : m_diskPartitionList){
            PdmUtils::removeDirRecursive(partition->getMountName());
        }
        m_storageDeviceHandlerCb(REMOVE,nullptr);
        PDM_LOG_INFO("StorageDevice:",0,"%s line: %d : m_isSdCardRemoved :  %d", __FUNCTION__,__LINE__,m_isSdCardRemoved);
         m_isSdCardRemoved = true;
    }
}


bool StorageDevice::triggerUevent() {

    if(m_isSdCardRemoved ==  false)
        return false;
    PDM_LOG_INFO("StorageDevice:",0,"%s line: %d : DISK_MEDIA_CHANGE is not preset", __FUNCTION__,__LINE__);
    m_isSdCardRemoved = false;
    std::string command = "udevadm trigger --name-match=/dev/";
    command.append(m_deviceName);
    PdmUtils::execShellCmd(command);
    return true;
}

void StorageDevice::setExtraSdCardDetails(IDevice &device) {

    m_usbPortNum = device.getUsbPortNumber();
    m_devSpeed = device.getDevSpeed();
    m_deviceNum = device.getDeviceNum();
    m_deviceNum *= 10;
}
