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

#include "PdmJson.h"
#include "PTPDeviceHandler.h"

using namespace PdmDevAttributes;

bool PTPDeviceHandler::mIsObjRegistered = PTPDeviceHandler::RegisterObject();

PTPDeviceHandler::PTPDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) : DeviceHandler(pConfObj, pluginAdapter){
    lunaHandler->registerLunaCallback(std::bind(&PTPDeviceHandler::GetAttachedDeviceStatus, this, _1, _2),GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&PTPDeviceHandler::GetAttachedStorageDeviceList, this, _1, _2), GET_STORAGEDEVICELIST);
    lunaHandler->registerLunaCallback(std::bind(&PTPDeviceHandler::GetAttachedStorageDeviceList, this, _1, _2), GET_EXAMPLE);
}

PTPDeviceHandler::~PTPDeviceHandler() {
}

bool PTPDeviceHandler::HandlerEvent(PdmNetlinkEvent* pNE){

    PDM_LOG_DEBUG("PTPDeviceHandler::HandlerEvent");

   if (pNE->getDevAttribute(ACTION) == "remove")
   {
      ProcessPTPDevice(pNE);
      return false;
   }
    std::string interfaceClass = pNE->getInterfaceClass();
    if(interfaceClass.find(iClass) == std::string::npos)
        return false;
    if((pNE->getDevAttribute(DEVTYPE) ==  USB_DEVICE) && (pNE->getDevAttribute(ID_MEDIA_PLAYER) != "1")){
        ProcessPTPDevice(pNE);
        return false;
    }
    else if((pNE->getDevAttribute(SUBSYSTEM) ==  "usb") && (pNE->getDevAttribute(ID_MEDIA_PLAYER) != "1")) {
        ProcessPTPDevice(pNE);
        return true;
    }
    return false;
}

void PTPDeviceHandler::removeDevice(PTPDevice* ptpDevice)
{
    if(!ptpDevice)
        return;
    sList.remove(ptpDevice);
    Notify(PTP_DEVICE, REMOVE, ptpDevice);
    delete ptpDevice;
    ptpDevice = nullptr;
}

void PTPDeviceHandler::ProcessPTPDevice(PdmNetlinkEvent* pNE) {
    PTPDevice *ptpDevice;
    PDM_LOG_INFO("PTPDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__,__LINE__,pNE->getDevAttribute(DEVTYPE).c_str(),pNE->getDevAttribute(ACTION).c_str());
    try {
            switch(sMapDeviceActions.at(pNE->getDevAttribute(ACTION)))
            {
                case DeviceActions::USB_DEV_ADD:
                    PDM_LOG_DEBUG("PTPDeviceHandler:%s line: %d action : %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(ACTION).c_str());
                    ptpDevice = new (std::nothrow) PTPDevice(m_pConfObj, m_pluginAdapter);
                    if(!ptpDevice) {
                        PDM_LOG_CRITICAL("PTPDeviceHandler:%s line: %d Unable to create new PTP Device", __FUNCTION__, __LINE__);
                        return;
                    }
                    ptpDevice->setDeviceInfo(pNE);
                    if(ptpDevice->getIsMounted()){
                        ptpDevice->registerCallback(std::bind(&PTPDeviceHandler::commandNotification, this, _1, _2));
                        sList.push_back(ptpDevice);
                        Notify(PTP_DEVICE, ADD);
                    } else {
                       PDM_LOG_CRITICAL("PTPDeviceHandler:%s line: %d Unable to mount PTP Device removing", __FUNCTION__, __LINE__);
                       ptpDevice->onDeviceRemove();
                       delete ptpDevice;
                    }
                    break;
                case DeviceActions::USB_DEV_REMOVE:
                    PDM_LOG_DEBUG("PTPDeviceHandler:%s line: %d action : %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(ACTION).c_str());
                    ptpDevice = getDeviceWithPath<PTPDevice>(sList,pNE->getDevAttribute(DEVPATH));
                    if(ptpDevice) {
                        ptpDevice->onDeviceRemove();
                        removeDevice(ptpDevice);
                    }
                    break;
                default:
                 //Do nothing
                    break;
            }
        }
        catch (const std::out_of_range& err) {
         PDM_LOG_INFO("PTPDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }
}

bool PTPDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    PDM_LOG_DEBUG("PTPDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    bool result = false;

    if(!cmdtypes)
        return result;

    if(cmdtypes->commandId == EJECT)
        result = eject(cmdtypes, cmdResponse);
    return result;
}

bool PTPDeviceHandler::eject(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    EjectCommand *ejectcmd = reinterpret_cast<EjectCommand*>(cmdtypes);
    PDM_LOG_INFO("PTPDeviceHandler:",0,"%s line: %d deviceNum:%d", __FUNCTION__,__LINE__,ejectcmd->deviceNumber);
    PdmDevStatus result =  PdmDevStatus::PDM_DEV_DEVICE_NOT_FOUND;
    bool ret = false;
    PTPDevice *ptpDevice = getDeviceWithNum<PTPDevice>(sList,ejectcmd->deviceNumber);
    if(ptpDevice){
        result = ptpDevice->eject();
        Notify(PTP_DEVICE,UMOUNT);
        ret = true;
    }
    commandResponse(cmdResponse,result);
    return ret;
}

bool PTPDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedStreamingDeviceStatus< PTPDevice >(sList, payload );
}

bool PTPDeviceHandler::GetAttachedStorageDeviceList (pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedStorageDeviceList< PTPDevice >(sList, payload );
}

bool PTPDeviceHandler::HandlePluginEvent(int eventType) {

    bool retVal = false;

    switch(eventType) {
    case POWER_PROCESS_REQEUST_SUSPEND:
        suspendRequest();
        break;
    case POWER_PROCESS_PREPARE_RESUME:
        resumeRequest(eventType);
        break;
    case POWER_PROCESS_REQEUST_UMOUNTALL:
    case POWER_PROCESS_PREPARE_SUSPEND:
        retVal = umountAllDrive();
        break;
    }

    return retVal;
}

bool PTPDeviceHandler::umountAllDrive() {
    bool retVal = true;
    for(auto ptpDevice : sList) {

        if(ptpDevice->unmountDevice() == false)
            retVal = false;
    }

    return retVal;
}

void PTPDeviceHandler::suspendRequest() {
    for(auto ptpDevice : sList)
        ptpDevice->setPowerStatus(false);
}

void PTPDeviceHandler::resumeRequest(const int &eventType) {
    for(auto ptpDevice : sList)
        ptpDevice->resumeRequest(eventType);
}

void PTPDeviceHandler::commandNotification(EventType event, PTPDevice* device)
{
    if(event == MOUNT || event == UMOUNT)
        Notify(ALL_DEVICE, event, device);
    else
        Notify(PTP_DEVICE, event, device);
}
