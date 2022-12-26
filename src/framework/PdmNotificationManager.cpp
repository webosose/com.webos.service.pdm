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
#include <sys/shm.h>

//notification Icons
    const std::string DEVICE_CONNECTED_ICON_PATH = "/usr/share/physical-device-manager/usb_connect.png";

//Alert IDs
    const std::string ALERT_ID_USB_STORAGE_DEV_REMOVED = "usbStorageDevRemoved";
    const std::string ALERT_ID_USB_STORAGE_DEV_UNSUPPORTED_FS = "usbStorageDevUnsupportedFs";
    const std::string ALERT_ID_USB_MAX_STORAGE_DEVCIES = "usbMaxStorageDevices";
    const std::string ALERT_ID_USB_STORAGE_FSCK_TIME_OUT = "usbStorageDevicesFsckTimeOut";

// Alert strings
    const std::string ALERT_STRING_USB_STORAGE_DEV_UNSUPPORTED_FS = "This USB storage has an unsupported system and cannot be read.";

PdmNotificationManager::PdmNotificationManager()
     : IObserver(), m_powerState(true)
{
    m_pLocHandler = PdmLocaleHandler::getInstance();
}

PdmNotificationManager::~PdmNotificationManager()
{
    std::list<DeviceHandler*> pDeviceHandlerList = DeviceManager::getInstance()->getDeviceHandlerList();

    for(auto handler : pDeviceHandlerList)
        handler->Unregister(this);
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
            if(eventDeviceType ==PdmDevAttributes::MTP_DEVICE)
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

int PdmNotificationManager::getPIDbyName(char *pidName, size_t len)
{
    FILE *fp;
    char pidofCmd[len + 18] = {0};
    int pidValue = -1;

    if (pidName != 0) {
        strncpy(pidofCmd, "pidof ", 6);
        strncat(pidofCmd, pidName, len);
        strncat(pidofCmd, "> /tmp/pidof", 12);
        system(pidofCmd);
        fp = fopen("/tmp/pidof", "r");
        if (fscanf(fp, "%d", &pidValue) != 1)
            pidValue = -1;
        fclose(fp);
        unlink("/tmp/pidof");
    }
    return pidValue;
}

void PdmNotificationManager::sendAlertInfo(pdmEvent pEvent, pbnjson::JValue parameters)
{
    int shmId;
    int eventPid = 0;
    char *sharedMemory;
    std::string payloadStr;
    pbnjson::JValue payload = pbnjson::Object();
    union sigval sv;

    payload.put("pdmEvent", pEvent);
    payload.put("parameters", parameters);
    payloadStr = payload.stringify().c_str();
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d payload for signal handler: %s", __FUNCTION__, __LINE__, payload.stringify().c_str());

    shmId = shmget(PDM_SHM_KEY, payloadStr.length(), 0666 | IPC_CREAT);
    if (shmId == -1)
        PDM_LOG_ERROR("shmget() is failed error :%s", strerror(errno));

    sharedMemory = (char *)shmat(shmId, (void *)0, 0);

    if(sharedMemory != nullptr) {

        memcpy(sharedMemory, payloadStr.c_str(), payloadStr.length());
        shmdt(sharedMemory);

        sv.sival_int = payloadStr.length();
        // passed 13 as event-monitor string length
        eventPid = PdmNotificationManager::getPIDbyName("event-monitor", 13);
        PDM_LOG_DEBUG("Event-monitor process ID :%d", eventPid);

        if (eventPid > 0) {
            if (-1 == sigqueue(eventPid, SIGUSR2, sv))
                PDM_LOG_ERROR("sigqueue is failed error :%s", strerror(errno));
        }
    }
}

void PdmNotificationManager::showConnectingToast(int eventDeviceType)
{
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d device type: %d", __FUNCTION__, __LINE__,eventDeviceType);

    switch(eventDeviceType)
    {
    case PdmDevAttributes::STORAGE_DEVICE:
        pbnjson::JValue parameters = pbnjson::Object();
        PdmNotificationManager::sendAlertInfo(CONNECTING_EVENT, parameters);
        break;
    }
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
    pbnjson::JValue buttons = pbnjson::JArray
    {
        pbnjson::JObject{{"label", m_pLocHandler->getLocString("OK")}}
    };
    std::string driveName = pMtpDev->getDriveName();
    std::string alertIdDevRemoved = ALERT_ID_USB_STORAGE_DEV_REMOVED + driveName;
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d alertId: %s", __FUNCTION__, __LINE__, alertIdDevRemoved.c_str());
    createAlert(alertIdDevRemoved,m_pLocHandler->getLocString("After removing, please reconnect the usb device."),buttons);
}

void PdmNotificationManager::createAlertForUnmountedDeviceRemoval(IDevice* device)
{
    if(!device)
        return;
    StorageDevice*  pStorageDev = dynamic_cast<StorageDevice*>(device);
    if(!pStorageDev)
        return;

    std::string devNumStr = std::to_string(pStorageDev->getDeviceNum());
    std::string alertIdFsckTimeout = ALERT_ID_USB_STORAGE_FSCK_TIME_OUT + devNumStr;
    closeAlert(alertIdFsckTimeout);

    pbnjson::JValue buttons = pbnjson::JArray
    {
        pbnjson::JObject{{"label", m_pLocHandler->getLocString("OK")}}
    };

    std::string alertIdDevRemoved = ALERT_ID_USB_STORAGE_DEV_REMOVED + devNumStr;
    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d alertId: %s", __FUNCTION__, __LINE__, alertIdDevRemoved.c_str());

    createAlert(alertIdDevRemoved,m_pLocHandler->getLocString("After removing, please reconnect the usb device."),buttons);
}

void PdmNotificationManager::createAlertForUnsupportedFileSystem(IDevice* device)
{
    if(!device)
        return;
    StorageDevice*  pStorageDev = dynamic_cast<StorageDevice*>(device);
    if(!pStorageDev)
        return;

    std::string devNumStr = std::to_string(pStorageDev->getDeviceNum());
    std::string alertIdUnsupportedFs = ALERT_ID_USB_STORAGE_DEV_UNSUPPORTED_FS + devNumStr;

    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d Input list is NOT supported", __FUNCTION__, __LINE__);
    pbnjson::JValue buttons = pbnjson::JArray
    {
        pbnjson::JObject{{"label", m_pLocHandler->getLocString("OK")}}
    };
    createAlert(alertIdUnsupportedFs,m_pLocHandler->getLocString(ALERT_STRING_USB_STORAGE_DEV_UNSUPPORTED_FS),buttons);

    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d alertId: %s", __FUNCTION__, __LINE__, alertIdUnsupportedFs.c_str());
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
    std::string alertIdFsckTimeout = ALERT_ID_USB_STORAGE_FSCK_TIME_OUT + devNumStr;
    std::string mountName = pStorageDev->getDeviceName();
    pbnjson::JValue buttons = pbnjson::JArray
    {
        pbnjson::JObject{{"label", m_pLocHandler->getLocString("CHECK & REPAIR")},
        {"onclick", "luna://com.webos.service.pdm/mountandFullFsck"},
        {"params", pbnjson::JObject{{"needFsck",true},{"mountName", mountName}}}},
        pbnjson::JObject{{"label", m_pLocHandler->getLocString("OPEN NOW")},
        {"onclick", "luna://com.webos.service.pdm/mountandFullFsck"},
        {"params", pbnjson::JObject{{"needFsck",false},{"mountName", mountName}}}}
    };

    PDM_LOG_DEBUG("PdmNotificationManager:%s line: %d alertId: %s", __FUNCTION__, __LINE__, alertIdFsckTimeout.c_str());

    createAlert(alertIdFsckTimeout,m_pLocHandler->getLocString("Some files may not be recognizable. Do you want to open device name now?"),buttons);
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

    IString *istr = new IString(m_pLocHandler->getLocString("Formatting {DRIVEINFO}..."));
    map<string, string> ivalues;
    ivalues.insert(pair<string, string>("DRIVEINFO", driveInfo));

    showToast(istr->format(ivalues),DEVICE_CONNECTED_ICON_PATH);
    delete istr;
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

    IString *istr = new IString(m_pLocHandler->getLocString("Formatting {DRIVEINFO} has been successfully completed."));
    map<string, string> ivalues;
    ivalues.insert(pair<string, string>("DRIVEINFO", driveInfo));

    showToast(istr->format(ivalues),DEVICE_CONNECTED_ICON_PATH);
    delete istr;
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

    IString *istr = new IString(m_pLocHandler->getLocString("Formatting {DRIVEINFO} has not been successfully completed."));
    map<string, string> ivalues;
    ivalues.insert(pair<string, string>("DRIVEINFO", driveInfo));

    showToast(istr->format(ivalues),DEVICE_CONNECTED_ICON_PATH);
    delete istr;
}

void PdmNotificationManager::closeUnsupportedFsAlert(IDevice* device)
{
    if(!device)
        return;
    StorageDevice*  pStorageDev = dynamic_cast<StorageDevice*>(device);
    if(!pStorageDev)
        return;

    std::string devNumStr = std::to_string(pStorageDev->getDeviceNum());
    std::string alertIdUnsupportedFs = ALERT_ID_USB_STORAGE_DEV_UNSUPPORTED_FS + devNumStr;

    closeAlert(alertIdUnsupportedFs);
}
