// Copyright (c) 2019-2022 LG Electronics, Inc.
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

#include <sstream>
#include <iomanip>
#include <unordered_map>

#include "Common.h"
#include "DeviceHandler.h"
#include "DeviceManager.h"
#include "PdmNotificationManager.h"
#include "PdmLogUtils.h"
#include "MTPDevice.h"
#include "StorageDevice.h"
#include "DiskPartitionInfo.h"
#include "PdmUtils.h"
#include <sys/shm.h>

PdmNotificationManager::PdmNotificationManager()
     : IObserver(), m_powerState(true)
{
    m_pLocHandler = PdmLocaleHandler::getInstance();
    m_shmId = shmget(PDM_SHM_KEY, 256, 0640 | IPC_CREAT);
    if (m_shmId == -1)
        PDM_LOG_ERROR("shmget() is failed, error :%s", strerror(errno));
}

PdmNotificationManager::~PdmNotificationManager()
{
    std::list<DeviceHandler*> pDeviceHandlerList = DeviceManager::getInstance()->getDeviceHandlerList();

    for(auto handler : pDeviceHandlerList)
        handler->Unregister(this);

    if (shmctl(m_shmId, IPC_RMID, NULL) == -1)
        PDM_LOG_ERROR("shmctl() is failed, error :%s", strerror(errno));
}

void PdmNotificationManager::attachObservers()
{
    std::list<DeviceHandler*> pDeviceHandlerList = DeviceManager::getInstance()->getDeviceHandlerList();

    for(auto handler : pDeviceHandlerList)
        handler->Register(this);
}

bool PdmNotificationManager::HandlePluginEvent(int eventType)
{
    PDM_LOG_DEBUG("PdmNotificationManager: %s line: %d Event Type %d", __FUNCTION__, __LINE__, eventType );
    switch(eventType)
    {
        case POWER_PROCESS_REQEUST_SUSPEND:
        case POWER_PROCESS_PREPARE_RESUME:
            m_powerState = false;
            break;
        case POWER_STATE_RESUME_DONE:
            m_powerState = true;
            break;
        default:
            //Nothing
            break;
    }
    return false;
}

void PdmNotificationManager::update(const int &eventDeviceType, const int &eventID, IDevice* device)
{
    PDM_LOG_INFO("PdmNotificationManager:",0,"%s line: %d update -  Event: %d eventID: %d", __FUNCTION__,__LINE__,eventDeviceType, eventID);
    if(m_powerState == false)
        return;

    if((device) && (!device->canDisplayToast()))
         return;

    switch(eventID)
    {
        case CONNECTING: showConnectingToast(eventDeviceType); break;
        case MAX_COUNT_REACHED: createAlertForMaxUsbStorageDevices();break;
        case REMOVE_BEFORE_MOUNT:
            if(eventDeviceType == PdmDevAttributes::MTP_DEVICE)
            {
                unMountMtpDeviceAlert(device);
                break;
            }else{
                createAlertForUnmountedDeviceRemoval(device);
                break;
            }
        case UNSUPPORTED_FS_FORMAT_NEEDED: createAlertForUnsupportedFileSystem(device);break;
        case FSCK_TIMED_OUT: createAlertForFsckTimeout(device);break;
        case FORMAT_STARTED: showFormatStartedToast(device);break;
        case FORMAT_SUCCESS: showFormatSuccessToast(device);break;
        case FORMAT_FAIL: showFormatFailToast(device);break;
        case REMOVE_UNSUPPORTED_FS: closeUnsupportedFsAlert(device);break;
    }
}

bool PdmNotificationManager::isToastRequired(int eventDeviceType)
{
    switch(eventDeviceType)
    {
        default:
        //Except above devices,Toast is required for all other device.
            return true;
    }
}

void PdmNotificationManager::sendAlertInfo(pdmEvent pEvent, pbnjson::JValue parameters)
{
    int shmId;
    unsigned int eventPid = 0;
    char *sharedMemory;
    std::string payloadStr;
    pbnjson::JValue payload = pbnjson::Object();
    union sigval sv;

    payload.put("pdmEvent", pEvent);
    payload.put("parameters", parameters);
    payloadStr = payload.stringify().c_str();
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d payload for signal handler: %s", __FUNCTION__, __LINE__, payload.stringify().c_str());

    shmId = shmget(PDM_SHM_KEY, payloadStr.length(), 0);
    if (shmId == -1) {
        PDM_LOG_ERROR("shmget() is failed error :%s", strerror(errno));
        return;
    }

    sharedMemory = (char *)shmat(shmId, (void *)0, 0);

    if(sharedMemory != nullptr) {

        memcpy(sharedMemory, payloadStr.c_str(), payloadStr.length());
        shmdt(sharedMemory);

        sv.sival_int = payloadStr.length();

        eventPid = PdmUtils::getPIDbyName("event-monitor");
        PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d  event-monitor process ID :%d", __FUNCTION__, __LINE__, eventPid);

        if (eventPid > 0) {
            if (-1 == sigqueue(eventPid, SIGUSR2, sv))
                PDM_LOG_ERROR("sigqueue is failed error :%s", strerror(errno));
        }
    }
}

void PdmNotificationManager::showConnectingToast(int eventDeviceType)
{
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d device type: %d", __FUNCTION__, __LINE__, eventDeviceType);

    pbnjson::JValue parameters = pbnjson::Object();
    parameters.put("deviceType", eventDeviceType);
    PdmNotificationManager::sendAlertInfo(CONNECTING_EVENT, parameters);
}

void PdmNotificationManager::showToast(const std::string& message,const std::string &iconUrl)
{
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d ", __FUNCTION__, __LINE__);

    if(!createToast(message,iconUrl))
        PDM_LOG_ERROR("Unable to create Toast");
}

void PdmNotificationManager::createAlertForMaxUsbStorageDevices()
{
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d ", __FUNCTION__, __LINE__);

    pbnjson::JValue parameters = pbnjson::Object();
    PdmNotificationManager::sendAlertInfo(MAX_COUNT_REACHED_EVENT, parameters);
}

void PdmNotificationManager::unMountMtpDeviceAlert(IDevice* device)
{
    if(!device)
    {
        cout << "Path Matched"<< endl;
        return;
    }

    MTPDevice*  pMtpDev = dynamic_cast<MTPDevice*>(device);
    if(!pMtpDev)
        return;

    std::string driveName = pMtpDev->getDriveName();
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d driveName: %s", __FUNCTION__, __LINE__, driveName.c_str());
    pbnjson::JValue parameters = pbnjson::Object();
    parameters.put("driveName", driveName);
    PdmNotificationManager::sendAlertInfo(REMOVE_BEFORE_MOUNT_MTP_EVENT, parameters);
}

void PdmNotificationManager::createAlertForUnmountedDeviceRemoval(IDevice* device)
{
    if(!device)
        return;

    StorageDevice*  pStorageDev = dynamic_cast<StorageDevice*>(device);
    if(!pStorageDev)
        return;

    std::string devNumStr = std::to_string(pStorageDev->getDeviceNum());

    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d devNumStr: %s", __FUNCTION__, __LINE__, devNumStr.c_str());

    pbnjson::JValue parameters = pbnjson::Object();
    parameters.put("deviceNum", devNumStr);
    PdmNotificationManager::sendAlertInfo(REMOVE_BEFORE_MOUNT_EVENT, parameters);
}

void PdmNotificationManager::createAlertForUnsupportedFileSystem(IDevice* device)
{
    if(!device)
        return;

    StorageDevice*  pStorageDev = dynamic_cast<StorageDevice*>(device);
    if(!pStorageDev)
        return;

    std::string devNumStr = std::to_string(pStorageDev->getDeviceNum());

    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d devNumStr: %s", __FUNCTION__, __LINE__, devNumStr.c_str());

    pbnjson::JValue parameters = pbnjson::Object();
    parameters.put("deviceNum", devNumStr);
    PdmNotificationManager::sendAlertInfo(UNSUPPORTED_FS_FORMAT_NEEDED_EVENT, parameters);
}

void PdmNotificationManager::createAlertForFsckTimeout(IDevice* device)
{
    PDM_LOG_WARNING("PdmNotificationManager:%s line: %d Creating alert for fsck timeout", __FUNCTION__, __LINE__);

    if(!device)
        return;

    StorageDevice*  pStorageDev = dynamic_cast<StorageDevice*>(device);
    if(!pStorageDev)
        return;

    std::string devNumStr = std::to_string(pStorageDev->getDeviceNum());
    std::string mountName = pStorageDev->getDeviceName();

    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d devNumStr: %s mountName: %s", __FUNCTION__, __LINE__, devNumStr.c_str(), mountName.c_str());

    pbnjson::JValue parameters = pbnjson::Object();
    parameters.put("deviceNum", devNumStr);
    parameters.put("mountName", mountName);
    PdmNotificationManager::sendAlertInfo(FSCK_TIMED_OUT_EVENT, parameters);
}

void PdmNotificationManager::showFormatStartedToast(IDevice* device)
{
    if(!device)
        return;

    DiskPartitionInfo*  pPartition = dynamic_cast<DiskPartitionInfo*>(device);
    if(!pPartition)
        return;

    std::stringstream driveSizeInGb;
    driveSizeInGb << std::fixed << std::setprecision(2) << pPartition->getDriveSize()/(float)(1024 * 1024);
    std::string driveInfo = "[" + driveSizeInGb.str() + "GB] " + pPartition->getProductName();
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d driveInfo: %s", __FUNCTION__, __LINE__, driveInfo.c_str());

    pbnjson::JValue parameters = pbnjson::Object();
    parameters.put("driveInfo", driveInfo);
    PdmNotificationManager::sendAlertInfo(FORMAT_STARTED_EVENT, parameters);
}

void PdmNotificationManager::showFormatSuccessToast(IDevice* device)
{
    if(!device)
        return;

    DiskPartitionInfo*  pPartition = dynamic_cast<DiskPartitionInfo*>(device);
    if(!pPartition)
        return;

    std::stringstream driveSizeInGb;
    driveSizeInGb << std::fixed << std::setprecision(2) << pPartition->getDriveSize()/(float)(1024 * 1024);
    std::string driveInfo = "[" + driveSizeInGb.str() + "GB] " + pPartition->getProductName();
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d driveInfo: %s", __FUNCTION__, __LINE__, driveInfo.c_str());

    pbnjson::JValue parameters = pbnjson::Object();
    parameters.put("driveInfo", driveInfo);
    PdmNotificationManager::sendAlertInfo(FORMAT_SUCCESS_EVENT, parameters);
}

void PdmNotificationManager::showFormatFailToast(IDevice* device)
{
    if(!device)
        return;

    DiskPartitionInfo*  pPartition = dynamic_cast<DiskPartitionInfo*>(device);
    if(!pPartition)
        return;

    std::stringstream driveSizeInGb;
    driveSizeInGb << std::fixed << std::setprecision(2) << pPartition->getDriveSize()/(float)(1024 * 1024);
    std::string driveInfo = "[" + driveSizeInGb.str() + "GB] " + pPartition->getProductName();
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d driveInfo: %s", __FUNCTION__, __LINE__, driveInfo.c_str());

    pbnjson::JValue parameters = pbnjson::Object();
    parameters.put("driveInfo", driveInfo);
    PdmNotificationManager::sendAlertInfo(FORMAT_FAIL_EVENT, parameters);
}

void PdmNotificationManager::closeUnsupportedFsAlert(IDevice* device)
{
    if(!device)
        return;

    StorageDevice*  pStorageDev = dynamic_cast<StorageDevice*>(device);
    if(!pStorageDev)
        return;

    std::string devNumStr = std::to_string(pStorageDev->getDeviceNum());

    pbnjson::JValue parameters = pbnjson::Object();
    parameters.put("deviceNum", devNumStr);
    PdmNotificationManager::sendAlertInfo(REMOVE_UNSUPPORTED_FS_EVENT, parameters);
}
