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

#include "StorageDeviceHandler.h"
#include "PdmErrors.h"
#include "PdmLogUtils.h"
#include "PdmJson.h"
#include <algorithm>

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

bool StorageDeviceHandler::HandlerEvent(PdmNetlinkEvent* pNE){

    PDM_LOG_DEBUG("StorageDeviceHandler::HandlerEvent");

    if(!isStorageDevice(pNE))
        return false;
    if(pNE->getDevAttribute(DEVTYPE) ==  USB_DEVICE) {
        ProcessStorageDevice(pNE);
        return false;
    }
    else if(pNE->getDevAttribute(SUBSYSTEM) ==  "block") {
        ProcessStorageDevice(pNE);
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
        case UMOUNT_ALL_DRIVE:
            result = umountAllDrive(cmdResponse);
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
        case IO_PERFORMANCE:
            result = getIoPerf(cmdtypes, cmdResponse);
            break;
        default:
            PDM_LOG_CRITICAL("Command not supported");
            break;
    }

    return result;
}

void StorageDeviceHandler::ProcessStorageDevice(PdmNetlinkEvent* pNE) {
    PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__,__LINE__,pNE->getDevAttribute(DEVTYPE).c_str(),pNE->getDevAttribute(ACTION).c_str());
    switch(sMapDeviceActions[pNE->getDevAttribute(ACTION)])
    {
        case DeviceActions::USB_DEV_ADD:
        case DeviceActions::USB_DEV_CHANGE:
            checkStorageDevice(pNE);
            break;
        case DeviceActions::USB_DEV_REMOVE:
            deleteStorageDevice(pNE);
            break;
        default:
            //Do nothing
            break;
    }
}

void StorageDeviceHandler::checkStorageDevice(PdmNetlinkEvent* pNE) {

    StorageDevice *storageDevPath = getDeviceWithPath< StorageDevice >(mStorageList,pNE->getDevAttribute(DEVPATH));
    if((pNE->getDevAttribute(DEVTYPE) == USB_DEVICE) && (storageDevPath == nullptr)) {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d New device with path DEVNAME: %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVNAME).c_str());
        createStorageDevice(pNE, nullptr);
        return;
    }

    if(storageDevPath && storageDevPath->getDeviceName() == "") {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d updated device name empty DEVNAME: %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVNAME).c_str());
        storageDevPath->setDeviceInfo(pNE);
        return;
    }

    StorageDevice *storageDevName = getDeviceWithName< StorageDevice >(mStorageList, pNE->getDevAttribute(DEVNAME));

    if(storageDevName && storageDevPath) {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d update device with name DEVNAME: %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVNAME).c_str());
        storageDevName->setDeviceInfo(pNE);
        return;
    }
    if(storageDevName == nullptr) {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d new device with name DEVNAME: %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVNAME).c_str());
        createStorageDevice(pNE, storageDevPath);
        return;
    }

}

void StorageDeviceHandler::createStorageDevice(PdmNetlinkEvent* pNE, IDevice *device)
{
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d DEVNAME: %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVNUM).c_str());
    if(mStorageList.size() >= m_maxStorageDevices) {
        PDM_LOG_CRITICAL("StorageDeviceHandler:%s line: %d Storage Device count: %d has reached max. Dont process", __FUNCTION__, __LINE__, mStorageList.size());
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
    storageDev->setDeviceInfo(pNE);
    std::lock_guard<std::mutex> lock(mStorageListMtx);
    mStorageList.push_back(storageDev);
    PDM_LOG_DEBUG("StorageDeviceHandler: %s line: %d Storage Device count: %d ", __FUNCTION__,__LINE__,mStorageList.size());

}

void StorageDeviceHandler::deleteStorageDevice(PdmNetlinkEvent* pNE)
{
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d DEVNAME: %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVNUM).c_str());
    StorageDevice *storageDev = nullptr;

    if(pNE->getDevAttribute(DEVTYPE) == USB_DEVICE) {
           PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d with USB_DEVICE list size: %d", __FUNCTION__, __LINE__, mStorageList.size());
           storageDev = getDeviceWithPath < StorageDevice > (mStorageList, pNE->getDevAttribute(DEVPATH));
    } else if(pNE->getDevAttribute(DEVTYPE) == DISK) {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d with DISK list size: %d", __FUNCTION__, __LINE__, mStorageList.size());
        storageDev = getDeviceWithName < StorageDevice > (mStorageList, pNE->getDevAttribute(DEVNAME));
        if(!storageDev->getIsExtraSdCard())
            return;
    }
   if(nullptr == storageDev)
        return;
    storageDev->onDeviceRemove();
    removeDevice(storageDev);
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
bool StorageDeviceHandler::isStorageDevice(PdmNetlinkEvent* pNE)
{
    PDM_LOG_DEBUG("StorageDeviceHandler::isStorageDevice");
    std::string interfaceClass = pNE->getInterfaceClass();
    if((interfaceClass.find(iClass) != std::string::npos) || (pNE->getDevAttribute(HARD_DISK) == PDM_HDD_ID_ATA) ||
                                       (pNE->getDevAttribute(ID_ATA) == PDM_HDD_ID_ATA))
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
    for(auto storageDevice : mStorageList) {
        if(storageDevice->suspendUmountAllPartitions(lazyUnmount) == false)
            retVal = false;
    }
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

bool StorageDeviceHandler::getIoPerf(CommandType *cmdtypes, CommandResponse *cmdResponse){
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    bool retVal = true;
    IoPerformanceCommand *ioPerfCmd = reinterpret_cast<IoPerformanceCommand*>(cmdtypes);
    PdmDevStatus result = PdmDevStatus::PDM_DEV_ERROR;

    unsigned int chunkSize = ioPerfCmd->chunkSize;
    if (chunkSize == 0)
        chunkSize = IOPERF_CHUNKSIZE_DEFAULT;
    std::string mountName = ioPerfCmd->mountName;
    std::string driveName = ioPerfCmd->driveName;

    if(driveName.empty() && mountName.empty())
    {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d param error!\n", __FUNCTION__, __LINE__);
        commandResponse(cmdResponse,result);
        return retVal;
    }
    if(chunkSize < IOPERF_CHUNKSIZE_MIN || chunkSize > IOPERF_CHUNKSIZE_MAX)
    {
        PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d invalid chunk size!\n", __FUNCTION__, __LINE__);
        commandResponse(cmdResponse,result);
        return retVal;
    }

    PdmIoPerf perfIO;
    if(!driveName.empty())
    {
        StorageDevice* storageDev = getDeviceWithName<StorageDevice>(mStorageList,driveName);
        if(storageDev)
        {
            result = storageDev->ioPerf(driveName,chunkSize,&perfIO);
            if(result == PdmDevStatus::PDM_DEV_SUCCESS)
                logAndAppendPayloadForIOPerf(cmdResponse,&perfIO);
            else
            {
                result = PdmDevStatus::PDM_DEV_IO_PERF_FAIL;
                commandResponse(cmdResponse,result);
            }
            return retVal;
        }
        else
        {
            PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d drive not found\n", __FUNCTION__, __LINE__);
            result = PdmDevStatus::PDM_DEV_DRIVE_NOT_FOUND;
            commandResponse(cmdResponse,result);
            return retVal;
        }
    }

    result = perfIO.performIOTest(driveName,mountName,chunkSize);
    if(result == PdmDevStatus::PDM_DEV_SUCCESS)
        logAndAppendPayloadForIOPerf(cmdResponse,&perfIO);
    else
    {
        result = PdmDevStatus::PDM_DEV_IO_PERF_FAIL;
        commandResponse(cmdResponse,result);
    }
    return retVal;
}

void StorageDeviceHandler::logAndAppendPayloadForIOPerf(CommandResponse* cmdResponse, PdmIoPerf* perfIO)
{
    double seqAvgWrite = perfIO->getSeqAvgWrite();
    double seqMinRead = perfIO->getSeqMinRead();
    double ranMinRead = perfIO->getRanMinRead();
    double seqAvgRead = perfIO->getSeqAvgRead();
    double seqMinWrite = perfIO->getSeqMinWrite();
    double ranAvgRead = perfIO->getRanAvgRead();
    unsigned int chunkSize = perfIO->getChunkSize();
    std::string drvName = perfIO->getDrvName();
    std::string testPath = perfIO->getTestPath();

    pbnjson::JValue ioPerfInfo = pbnjson::Object();
    ioPerfInfo.put("seqWriteAverage", seqAvgWrite);
    ioPerfInfo.put("seqReadMinimum", seqMinRead);
    ioPerfInfo.put("randReadMinimum", ranMinRead);
    ioPerfInfo.put("seqReadAverage", seqAvgRead);
    ioPerfInfo.put("seqWriteMinimum", seqMinWrite);
    ioPerfInfo.put("randReadAverage", ranAvgRead);

    cmdResponse->cmdResponse = pbnjson::Object();
    cmdResponse->cmdResponse.put("ioPerformance",ioPerfInfo);
    cmdResponse->cmdResponse.put("returnValue", true);

    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d Storage random read test success!\n", __FUNCTION__, __LINE__);
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d ======== [drive name : %s, testpath : %s, chunk size : %d] IO Test Result \n", __FUNCTION__, __LINE__, drvName.c_str(), testPath.c_str(), chunkSize);
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d ======== sequencial average write : %lf MByte/s\n", __FUNCTION__, __LINE__, seqAvgWrite);
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d ======== sequencial min read : %lf MByte/s\n", __FUNCTION__, __LINE__, seqMinRead);
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d ======== random min read : %lf MByte/s\n", __FUNCTION__, __LINE__, ranMinRead);
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d ======== sequencial average read : %lf MByte/s\n", __FUNCTION__, __LINE__, seqAvgRead);
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d ======== sequencial min write : %lf MByte/s\n", __FUNCTION__, __LINE__, seqMinWrite);
    PDM_LOG_DEBUG("StorageDeviceHandler:%s line: %d ======== random average read : %lf MByte/s\n", __FUNCTION__, __LINE__, ranAvgRead);

}
