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

#include "StorageDeviceHandler.h"
#include "PdmErrors.h"
#include "PdmLogUtils.h"
#include "PdmJson.h"
#include <algorithm>
#include <luna-service2/lunaservice.hpp>
#include <luna-service2++/handle.hpp>
#include "LunaIPC.h"
#include <sys/mount.h>
#include "StorageSubsystem.h"

using namespace PdmDevAttributes;
using namespace std::placeholders;
//Timer for computeSpaceInfoThread
#define PDM_GETSPACEINFO_SLEEP_TIME 20
#define PDM_HDD_ID_ATA  "1"
#define PDM_PORT_SPEED_HIGH 102
#define PDM_PORT_SPEED_SUPER 103

#define IOPERF_CHUNKSIZE_DEFAULT 256
#define IOPERF_CHUNKSIZE_MIN 4
#define IOPERF_CHUNKSIZE_MAX 65536

const std::string iClass = ":08";

bool StorageDeviceHandler::mIsObjRegistered = StorageDeviceHandler::RegisterObject();

StorageDeviceHandler::StorageDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
            : DeviceHandler(pConfObj, pluginAdapter)
            , mSpaceInfoThreadStatus(false) {

    m_handlerName = "StorageHandler";
    m_maxStorageDevices = readMaxUsbStorageDevices();
    lunaHandler->registerLunaCallback(std::bind(&StorageDeviceHandler::GetAttachedDeviceStatus, this, _1, _2), GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&StorageDeviceHandler::GetAttachedStorageDeviceList, this, _1, _2), GET_STORAGEDEVICELIST);
    lunaHandler->registerLunaCallback(std::bind(&StorageDeviceHandler::GetExampleAttachedUsbStorageDeviceList, this, _1, _2), GET_EXAMPLE);
}

StorageDeviceHandler::~StorageDeviceHandler() {
    mSpaceInfoThreadStatus = false;
    mNotifyCv.notify_one();
    if(mSpaceInfoThread.joinable())
    {
        try {
            mSpaceInfoThread.join();
        }
        catch (std::system_error &e) {
            PDM_LOG_ERROR("StorageDeviceHandler:%s line: %d Caught system_error: %s", __FUNCTION__,__LINE__, e.what());
        }
    }
    if(!mStorageList.empty())
    {
        for( auto storageDev : mStorageList )
        {
            removeDevice(storageDev);
        }
        mStorageList.clear();
        PdmDeviceFactory::getInstance()->UnRegister("Storage");
        PDM_LOG_INFO("StorageDeviceHandler",0," Stoping....");
    }
}

int StorageDeviceHandler::readMaxUsbStorageDevices()
{
    int maxUsbStorageDevs = 0;
    pbnjson::JValue maxUsbStorageDevsConfVal = pbnjson::JValue();
    PdmConfigStatus confErrCode = m_pConfObj->getValue("Storage","MaxUsbStorageDevices",maxUsbStorageDevsConfVal);
    if(confErrCode != PdmConfigStatus::PDM_CONFIG_ERROR_NONE)
    {
        PdmErrors::logPdmErrorCodeAndText(confErrCode);
        return maxUsbStorageDevs;
    }
    if (maxUsbStorageDevsConfVal.isNumber())
    {
        maxUsbStorageDevs = maxUsbStorageDevsConfVal.asNumber<int>();
        PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d MaxUsbStorageDevices: %d", __FUNCTION__,__LINE__,maxUsbStorageDevs);
    }

    return maxUsbStorageDevs;
}

bool StorageDeviceHandler::HandlerEvent(DeviceClass* devClass)
{
    PDM_LOG_DEBUG("StorageDeviceHandler::HandlerEvent");
   if (devClass->getAction()== "remove")
   {
      if(deleteStorageDevice(devClass)) {
         PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d  DEVTYPE=usb_device removed", __FUNCTION__, __LINE__);
         return true;
      }
      return false;
   }
   if(!isStorageDevice(devClass))
        return false;
    if(devClass->getDevType() ==  USB_DEVICE) {
        ProcessStorageDevice(devClass);
        return false;
    }
    else if(devClass->getSubsystemName() ==  "block") {
        ProcessStorageDevice(devClass);
        return true;
    }
    return false;
}

bool StorageDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    bool result = false;

    if(!cmdtypes)
        return result;

    PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d Command Type: %d", __FUNCTION__,__LINE__,cmdtypes->commandId);
    switch(cmdtypes->commandId)
    {
#ifndef WEBOS_SESSION
        case FORMAT:
            result = format(cmdtypes, cmdResponse);
            break;
        case EJECT:
            result = eject(cmdtypes, cmdResponse);
            break;
        case FSCK:
            result = fsck(cmdtypes, cmdResponse);
            break;
        case SET_VOLUME_LABEL:
            result = setVolumeLabel(cmdtypes, cmdResponse);
            break;
        case IS_WRITABLE_DRIVE:
            result = isWritableDrive(cmdtypes, cmdResponse);
            break;
        case MOUNT_FSCK:
            result = mountFsck(cmdtypes, cmdResponse);
            break;
        case SPACE_INFO:
            result = getSpaceInfo(cmdtypes, cmdResponse);
            break;
#endif
        case UMOUNT_ALL_DRIVE:
            result = umountAllDrive(cmdResponse);
            break;
        default:
            PDM_LOG_CRITICAL("Command not supported");
            break;
    }

    return result;
}

void StorageDeviceHandler::ProcessStorageDevice(DeviceClass* devClass) {
    PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__,__LINE__, devClass->getDevType().c_str(), devClass->getAction().c_str());
    try {
        switch(sMapDeviceActions.at(devClass->getAction()))
         {
             case DeviceActions::USB_DEV_ADD:
             case DeviceActions::USB_DEV_CHANGE:
                 checkStorageDevice(devClass);
                 break;
             default:
                 //Do nothing
                 break;
        }
    }
      catch (const std::out_of_range& err) {
         PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }
}

void StorageDeviceHandler::checkStorageDevice(DeviceClass* devClass) {

    StorageDevice *storageDevPath = getDeviceWithPath< StorageDevice >(mStorageList, devClass->getDevPath());
    if((devClass->getDevType() == USB_DEVICE) && (storageDevPath == nullptr)) {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d New device with path DEVNAME: %s", __FUNCTION__, __LINE__, devClass->getDevName().c_str());
        createStorageDevice(devClass, nullptr);
        return;
    }

    if(storageDevPath && storageDevPath->getDeviceName() == "") {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d updated device name empty DEVNAME: %s", __FUNCTION__, __LINE__, devClass->getDevName().c_str());
        storageDevPath->setDeviceInfo(devClass);
        return;
    }

    StorageDevice *storageDevName = getDeviceWithName< StorageDevice >(mStorageList, devClass->getDevName());

    if(storageDevName && storageDevPath) {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d update device with name DEVNAME: %s", __FUNCTION__, __LINE__, devClass->getDevName().c_str());
        storageDevName->setDeviceInfo(devClass);
        return;
    }
    if(storageDevPath && storageDevName == nullptr) {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d new device with name DEVNAME: %s", __FUNCTION__, __LINE__, devClass->getDevName().c_str());
        createStorageDevice(devClass, storageDevPath);
        return;
    }

}

void StorageDeviceHandler::createStorageDevice(DeviceClass* devClass, IDevice *device)
{
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d DEVNAME: %s", __FUNCTION__, __LINE__, devClass->getDevName().c_str());
    if(mStorageList.size() >= m_maxStorageDevices) {
        PDM_LOG_CRITICAL("StorageDeviceHandler:%s line: %d Storage Device count: %zd has reached max. Dont process", __FUNCTION__, __LINE__, mStorageList.size());
        Notify(STORAGE_DEVICE,MAX_COUNT_REACHED);
        return;
    }
    StorageDevice *storageDev = new (std::nothrow) StorageDevice(m_pConfObj, m_pluginAdapter);
    if(nullptr == storageDev) {
         PDM_LOG_CRITICAL("StorageDeviceHandler:%s line: %d Unable to create new USB StorageDevice", __FUNCTION__, __LINE__);
         return;
    }
    storageDev->registerCallback(std::bind(&StorageDeviceHandler::commandNotification, this, _1, _2));
    if(device) {
        storageDev->setIsExtraSdCard(true);
        storageDev->setExtraSdCardDetails(*device);
    }
    storageDev->setDeviceInfo(devClass);
    std::lock_guard<std::mutex> lock(mStorageListMtx);
    mStorageList.push_back(storageDev);
    PDM_LOG_DEBUG("StorageDeviceHandler: %s line: %d Storage Device count: %zd ", __FUNCTION__,__LINE__,mStorageList.size());

}

bool StorageDeviceHandler::deleteStorageDevice(DeviceClass* devClass)
{
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d DEVNAME: %s", __FUNCTION__, __LINE__, devClass->getDevNumber().c_str());
    StorageDevice *storageDev = nullptr;
   bool deviceRemoveStatus =  false;

    if(devClass->getDevType() == USB_DEVICE) {
           PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d with USB_DEVICE list size: %zd", __FUNCTION__, __LINE__, mStorageList.size());
           storageDev = getDeviceWithPath < StorageDevice > (mStorageList, devClass->getDevPath());
         if(storageDev) {
            deviceRemoveStatus = true;
         }
    } else if(devClass->getDevType() == DISK) {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d with DISK list size: %zd", __FUNCTION__, __LINE__, mStorageList.size());
        storageDev = getDeviceWithName < StorageDevice > (mStorageList, devClass->getDevName());
        if(nullptr == storageDev){
            PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d with storageDev is null for DEVNAME: %s", __FUNCTION__, __LINE__, devClass->getDevName().c_str());
            return deviceRemoveStatus;
        }
        if(!storageDev->getIsExtraSdCard())
            return deviceRemoveStatus;
    }
   if(nullptr == storageDev)
        return deviceRemoveStatus;
    storageDev->onDeviceRemove();
    removeDevice(storageDev);
   return deviceRemoveStatus;
}

void StorageDeviceHandler::removeDevice(StorageDevice *storageDevice)
{
    if(storageDevice)
    {
        PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d", __FUNCTION__,__LINE__);
        std::lock_guard<std::mutex> lock(mStorageListMtx);
        mStorageList.remove(storageDevice);
        if(storageDevice->isDevAddNotified()) {
            PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d", __FUNCTION__,__LINE__);
            Notify(STORAGE_DEVICE, REMOVE, storageDevice);
        }
        PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d", __FUNCTION__,__LINE__);
        delete storageDevice;
        storageDevice = nullptr;
    }
}

bool StorageDeviceHandler::format(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    FormatCommand *formatcmd = reinterpret_cast<FormatCommand*>(cmdtypes);
    StorageDevice* storageDev = getDeviceWithName<StorageDevice>(mStorageList,formatcmd->driveName);
    PdmDevStatus result = PdmDevStatus::PDM_DEV_DRIVE_NOT_FOUND;
    bool ret = false;
    if(storageDev) {
        result = storageDev->formatDiskStart(formatcmd->driveName,formatcmd->fsType,formatcmd->volumeLabel);
        ret =  true;
    }
    commandResponse(cmdResponse,result);
    return ret;
}


bool StorageDeviceHandler::eject(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    EjectCommand *ejectcmd = reinterpret_cast<EjectCommand*>(cmdtypes);
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d deviceNum:%d", __FUNCTION__, __LINE__, ejectcmd->deviceNumber);
    StorageDevice *storageDev = getDeviceWithNum<StorageDevice>(mStorageList,ejectcmd->deviceNumber);
    PdmDevStatus result = PdmDevStatus::PDM_DEV_DEVICE_NOT_FOUND;
    bool ret = false;
    if(storageDev) {
        result = storageDev->eject();
        Notify(STORAGE_DEVICE,UNMOUNTALL);
        ret = true;
    }
    commandResponse(cmdResponse,result);
    return ret;
}

bool StorageDeviceHandler::fsck(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    FsckCommand *fsckcmd = reinterpret_cast<FsckCommand*>(cmdtypes);
    std::string driveName = fsckcmd->driveName;
    PDM_LOG_DEBUG("StorageDeviceHandler: %s line: %d driveName:%s", __FUNCTION__, __LINE__, driveName.c_str());
    std::string::iterator end_pos = std::remove(driveName.begin(), driveName.end(), ' ');
    driveName.erase(end_pos, driveName.end());
    PdmDevStatus result = PdmDevStatus::PDM_DEV_DRIVE_NOT_FOUND;
    bool ret = false;
    StorageDevice *storageDev = getDeviceWithName<StorageDevice>(mStorageList,driveName);
    if(storageDev) {
        PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d deviceName:%s", __FUNCTION__,__LINE__,storageDev->getDeviceName().c_str());
        result = storageDev->fsck(driveName);
        ret = true;
    }
    commandResponse(cmdResponse,result);
    return ret;
}

bool StorageDeviceHandler::umountAllDrive(CommandResponse *cmdResponse) {
    PdmDevStatus result = PdmDevStatus::PDM_DEV_SUCCESS;
    bool ret = false;
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);

    if(!mStorageList.empty()){
        for (std::list<StorageDevice*>::iterator it = mStorageList.begin(); it != mStorageList.end(); it++){
                PdmDevStatus tempResult = (*it)->umountAllPartition(false);
                if(tempResult != PdmDevStatus::PDM_DEV_SUCCESS) {
                    PDM_LOG_ERROR("StorageDeviceHandler:%s line: %d Fail to umount all drive", __FUNCTION__, __LINE__);
                    result = tempResult;
                }
        }
        Notify(STORAGE_DEVICE,UNMOUNTALL);
        ret = true;
    }
    commandResponse(cmdResponse,result);
    return ret;
}

bool StorageDeviceHandler::setVolumeLabel(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    VolumeLabelCommand *volumeLabelcmd = reinterpret_cast<VolumeLabelCommand*>(cmdtypes);

    PdmDevStatus result = PdmDevStatus::PDM_DEV_DRIVE_NOT_FOUND;
    bool ret= false;
    // Get the device in which the partition is present
    StorageDevice* storageDev = getDeviceWithName<StorageDevice>(mStorageList, volumeLabelcmd->driveName);
    if(storageDev) {
        result = storageDev->setPartitionVolumeLabel(volumeLabelcmd->driveName,volumeLabelcmd->volumeLabel);
        ret =  true;
    }
    commandResponse(cmdResponse,result);
    return ret;
}

bool StorageDeviceHandler::isWritableDrive(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    IsWritableCommand *isWritablecmd = reinterpret_cast<IsWritableCommand*>(cmdtypes);
    bool isWritable = false;
    PdmDevStatus result = PdmDevStatus::PDM_DEV_ERROR;
    bool ret = false;
    // Get the device in which the partition is present
    StorageDevice* storageDev = getDeviceWithName<StorageDevice>(mStorageList,isWritablecmd->driveName);
    if(storageDev) {
        result = storageDev->isWritable(isWritablecmd->driveName, isWritable);
        ret = true;
    }
    commandResponse(cmdResponse,result);
    if(result == PdmDevStatus::PDM_DEV_SUCCESS)
        cmdResponse->cmdResponse.put("isWritable", isWritable);
    return ret;
}

void StorageDeviceHandler::commandNotification(EventType event, Storage* device)
{
    if(event == MOUNT || event == UMOUNT)
        Notify(ALL_DEVICE,event,device);
    else
        Notify(STORAGE_DEVICE,event,device);

    if(mSpaceInfoThreadStatus == false)
    {
        if ( std::any_of(mStorageList.begin(), mStorageList.end(), [&](StorageDevice* dev){return dev->getIsMounted();}) )
        {
            if(mSpaceInfoThread.joinable())
            {
                mSpaceInfoThread.join();
            }
            mSpaceInfoThread = std::thread(&StorageDeviceHandler::computeSpaceInfoThread,this);
        }
    }
}

bool StorageDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedStorageDeviceStatus< StorageDevice >(mStorageList, payload );
}

bool StorageDeviceHandler::GetAttachedStorageDeviceList (pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedUsbStorageDeviceList< StorageDevice >(mStorageList, payload );
}

bool StorageDeviceHandler::GetExampleAttachedUsbStorageDeviceList (pbnjson::JValue &payload, LSMessage *message)
{
    return getExampleAttachedUsbStorageDeviceList< StorageDevice >(mStorageList, payload );
}

bool StorageDeviceHandler::getSpaceInfo (CommandType *cmdtypes, CommandResponse *cmdResponse)
{
    SpaceInfoCommand *spaceCmd = reinterpret_cast<SpaceInfoCommand*>(cmdtypes);
    StorageDevice* storageDev = getDeviceWithName<StorageDevice>(mStorageList,spaceCmd->driveName);

    if(!storageDev) {
        commandResponse(cmdResponse,PdmDevStatus::PDM_DEV_DRIVE_NOT_FOUND);
        return false;
    }

    DiskPartitionInfo* diskInfo = storageDev->getSpaceInfo(spaceCmd->driveName,spaceCmd->directCheck);
    if(diskInfo && diskInfo->isMounted()){
        commandResponse(cmdResponse,PdmDevStatus::PDM_DEV_SUCCESS);

        pbnjson::JValue detailedSpaceInfo = pbnjson::Object();
        detailedSpaceInfo.put("totalSize",(int32_t) diskInfo->getDriveSize());
        detailedSpaceInfo.put("freeSize", (int32_t) diskInfo->getFreeSize());
        detailedSpaceInfo.put("usedSize", (int32_t) diskInfo->getUsedSize());
        detailedSpaceInfo.put("usedRate", (int32_t) diskInfo->getUsedRate());
        cmdResponse->cmdResponse.put("spaceInfo",detailedSpaceInfo);
    } else {
        commandResponse(cmdResponse,PdmDevStatus::PDM_DEV_DRIVE_NOT_MOUNTED);
        PDM_LOG_WARNING("StorageDeviceHandler:%s line: %d driveName:%s Device not mounted", __FUNCTION__, __LINE__, spaceCmd->driveName.c_str());
    }
    return true;
}

void StorageDeviceHandler::computeSpaceInfoThread()
{
    mSpaceInfoThreadStatus = true;
    while(mSpaceInfoThreadStatus) {
        mStorageListMtx.lock();
        if(mStorageList.empty()) {
            PDM_LOG_WARNING("StorageDeviceHandler:%s List is empty so breaking the loop line: %d ", __FUNCTION__, __LINE__);
            mStorageListMtx.unlock();
            break;
        }
        if (std::any_of(mStorageList.begin(), mStorageList.end(), [&](StorageDevice* dev){return dev->getIsMounted();})) {
            for(auto storageDev : mStorageList) {
                if(storageDev->getIsMounted()) {
                    for(auto partition : storageDev->getDiskPartition()) {
                        storageDev->getSpaceInfo(partition->getDriveName(),true);
                    }
                }
            }
        } else {
            PDM_LOG_WARNING("StorageDeviceHandler:%s No device is mounted so breaking the loop line: %d ", __FUNCTION__, __LINE__);
            mStorageListMtx.unlock();
            break;
        }
        mStorageListMtx.unlock();
        std::unique_lock<std::mutex> lck(mNotifyMtx);
        mNotifyCv.wait_for(lck,std::chrono::seconds(PDM_GETSPACEINFO_SLEEP_TIME));
    }
    mSpaceInfoThreadStatus = false;
}

bool StorageDeviceHandler::isStorageDevice(DeviceClass* devClass)
{
    StorageSubsystem *storageSubsystem = (StorageSubsystem *)devClass;

    PDM_LOG_DEBUG("StorageDeviceHandler::isStorageDevice");
    std::string interfaceClass = devClass->getInterfaceClass();
    if((interfaceClass.find(iClass) != std::string::npos) || (storageSubsystem->getHardDisk() == PDM_HDD_ID_ATA) ||
                                       (storageSubsystem->getHardDisk() == PDM_HDD_ID_ATA))
        return true;

    return false;
}

bool StorageDeviceHandler::mountFsck(CommandType *cmdtypes, CommandResponse *cmdResponse)
{
    MountFsckCommand *formatcmd = reinterpret_cast<MountFsckCommand*>(cmdtypes);
    PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d mountName: %s, needFsck : %d", __FUNCTION__,__LINE__,formatcmd->mountName.c_str(),formatcmd->needFsck);
    PdmDevStatus result = PdmDevStatus::PDM_DEV_ERROR;
    bool ret=  false;
    StorageDevice* storageDev = getDeviceWithName<StorageDevice>(mStorageList,formatcmd->mountName);
    if(storageDev) {
        result  = storageDev->enforceFsckAndMount(formatcmd->needFsck);
        ret = true;
    }
    commandResponse(cmdResponse,result);
    return ret;
}


bool StorageDeviceHandler::HandlePluginEvent(int eventType) {

    switch(eventType) {
    case POWER_PROCESS_REQEUST_SUSPEND:
        suspendRequest();
        break;
    case POWER_PROCESS_PREPARE_RESUME:
        resumeRequest(eventType);
        break;
    case POWER_PROCESS_REQEUST_UMOUNTALL:
        // umount with force flag
        return umountAllDrive(false);
    case POWER_PROCESS_PREPARE_SUSPEND:
        //umount with deatch flag and force
        return umountAllDrive(true);
    default:
        break;
    }

    return false;

}

bool StorageDeviceHandler::umountAllDrive(bool lazyUnmount) {
    bool retVal = true;
#ifdef WEBOS_SESSION
    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue query;
    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                             {"where", pbnjson::JArray{{{"prop", "deviceType"}, {"op", "="}, {"val", "USB_STORAGE"}}}}};

    find_query.put("query", query);

    LS::Payload find_payload(find_query);
    LS::Call call = LunaIPC::getInstance()->getLSCPPHandle()->callOneReply("luna://com.webos.service.db/find", find_payload.getJson(), NULL, this, NULL);
    LS::Message message = call.get();

    LS::PayloadRef response_payload = message.accessPayload();
    pbnjson::JValue request = response_payload.getJValue();

    if(request.isNull())
    {
        PDM_LOG_ERROR("PdmLunaService::%s line: %d Nothing is there in Db ",  __FUNCTION__, __LINE__);
    }

    if(!request["returnValue"].asBool())
    {
        PDM_LOG_ERROR("PdmLunaService::%s line: %d not able to find from db",  __FUNCTION__, __LINE__);
    }

    for(ssize_t index = 0; index < request.arraySize() ; index++) {
        if(request[index]["deviceType"] == "USB_STORAGE") {
            for(ssize_t idx = 0; idx < request[index]["storageDriveList"].arraySize() ; idx++) {
                std::string mountName = request[index]["storageDriveList"][idx]["mountName"].asString();
                if(!mountName.empty()) {
                    int umountFlags = MNT_FORCE;

                    if(1)
                        umountFlags |= MNT_DETACH;

                    int res = umount2(mountName.c_str(), umountFlags);
                    if(res != 0) {
                        PDM_LOG_ERROR("PdmFs:%s line: %d Umount Failed :%s", __FUNCTION__, __LINE__, strerror(errno));
                        retVal = false;
                    }
                }
            }
        }
    }

#else
    for(auto storageDevice : mStorageList) {
        if(storageDevice->suspendUmountAllPartitions(lazyUnmount) == false)
            retVal = false;
    }
#endif
    return retVal;
}

void StorageDeviceHandler::suspendRequest() {

    for( auto storageDev : mStorageList  )
        storageDev->suspendRequest();
}

void StorageDeviceHandler::resumeRequest(const int &eventType) {
    PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d", __FUNCTION__,__LINE__);
    for( auto storageDev : mStorageList  )
        storageDev->resumeRequest(eventType);
    Notify(STORAGE_DEVICE, ADD);
}