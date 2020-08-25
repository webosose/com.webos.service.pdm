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

#include "MTPDeviceHandler.h"
#include "PdmJson.h"

using namespace PdmDevAttributes;

bool MTPDeviceHandler::mIsObjRegistered = MTPDeviceHandler::RegisterObject();

MTPDeviceHandler::MTPDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) : DeviceHandler(pConfObj, pluginAdapter){
    lunaHandler->registerLunaCallback(std::bind(&MTPDeviceHandler::GetAttachedDeviceStatus, this, _1, _2),GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&MTPDeviceHandler::GetAttachedStorageDeviceList, this, _1, _2), GET_STORAGEDEVICELIST);
    lunaHandler->registerLunaCallback(std::bind(&MTPDeviceHandler::GetAttachedStorageDeviceList, this, _1, _2), GET_EXAMPLE);
}

MTPDeviceHandler::~MTPDeviceHandler() {
}

bool MTPDeviceHandler::HandlerEvent(PdmNetlinkEvent* pNE){
#ifndef WEBOS_SESSION
   PDM_LOG_DEBUG("MTPDeviceHandler::HandlerEvent");
   if (pNE->getDevAttribute(ACTION) == "remove")
   {
      ProcessMTPDevice(pNE);
      return false;
   }else if (pNE->getDevAttribute(ID_MEDIA_PLAYER) == YES ){
        ProcessMTPDevice(pNE);
        return false;
    }
#endif
    return false;
}

void MTPDeviceHandler::removeDevice(MTPDevice* mtpDevice)
{
    if(!mtpDevice)
        return;
    mMtpList.remove(mtpDevice);
    Notify(MTP_DEVICE, REMOVE, mtpDevice);
    delete mtpDevice;
    mtpDevice = nullptr;
}

void MTPDeviceHandler::ProcessMTPDevice(PdmNetlinkEvent* pNE) {
   MTPDevice *mtpDevice;
    PDM_LOG_INFO("MTPDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__,__LINE__,pNE->getDevAttribute(DEVTYPE).c_str(),pNE->getDevAttribute(ACTION).c_str());
   try {
            switch(sMapDeviceActions.at(pNE->getDevAttribute(ACTION)))
            {
                case DeviceActions::USB_DEV_ADD:
                    PDM_LOG_DEBUG("MTPDeviceHandler:%s line: %d action : %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(ACTION).c_str());
                    if(!pNE->getDevAttribute(DEVNAME).empty()){
                        mtpDevice = new (std::nothrow) MTPDevice(m_pConfObj, m_pluginAdapter);
                        if(!mtpDevice)
                            break;
                        mtpDevice->setDeviceInfo(pNE);
                        mtpDevice->registerCallback(std::bind(&MTPDeviceHandler::commandNotification, this, _1, _2));
                        if(mtpDevice->mtpMount(pNE->getDevAttribute(DEVNAME)) == PdmDevStatus::PDM_DEV_SUCCESS){
                            mMtpList.push_back(mtpDevice);
                            Notify(MTP_DEVICE,ADD);
                        } else {
                      PDM_LOG_CRITICAL("MTPDeviceHandler:%s line: %d Unable to mount MTP Device removing", __FUNCTION__, __LINE__);
                      mtpDevice->onDeviceRemove();
                            delete mtpDevice;
                        }
                    }
                    break;
                case DeviceActions::USB_DEV_REMOVE:
                    PDM_LOG_DEBUG("MTPDeviceHandler:%s line: %d action : %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(ACTION).c_str());
                    mtpDevice = getDeviceWithPath<MTPDevice>(mMtpList,pNE->getDevAttribute(DEVPATH));
                    if(mtpDevice) {
                        mtpDevice->onDeviceRemove();
                        removeDevice(mtpDevice);
                    }
                    break;
                default:
                  //Do nothing
                    break;
            }
        }
        catch (const std::out_of_range& err) {
         PDM_LOG_INFO("MTPDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }
}

bool MTPDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    PDM_LOG_DEBUG("MTPDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    bool result = false;

    if(cmdtypes->commandId == EJECT)
        result = eject(cmdtypes, cmdResponse);
    return result;
}

bool MTPDeviceHandler::eject(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    EjectCommand *ejectcmd = reinterpret_cast<EjectCommand*>(cmdtypes);
    PDM_LOG_INFO("MTPDeviceHandler:",0,":%s line: %d deviceNum:%d", __FUNCTION__,__LINE__,ejectcmd->deviceNumber);
    PdmDevStatus result =  PdmDevStatus::PDM_DEV_DEVICE_NOT_FOUND;
    bool ret = false;
    MTPDevice *mtpDevice = getDeviceWithNum<MTPDevice>(mMtpList,ejectcmd->deviceNumber);
    if(mtpDevice){
        result = mtpDevice->eject();
        Notify(MTP_DEVICE,UMOUNT);
        ret = true;
    }
    commandResponse(cmdResponse,result);
    return ret;
}

bool MTPDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedDeviceStatus< MTPDevice >(mMtpList, payload );
}

bool MTPDeviceHandler::GetAttachedStorageDeviceList (pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedStorageDeviceList< MTPDevice >(mMtpList, payload );
}

void MTPDeviceHandler::commandNotification(EventType event, MTPDevice* device)
{
    if(event == MOUNT || event == UMOUNT)
        Notify(ALL_DEVICE,event,device);
    else
        Notify(MTP_DEVICE,event,device);
}


bool MTPDeviceHandler::HandlePluginEvent(int eventType) {

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

bool MTPDeviceHandler::umountAllDrive() {
    bool retVal = true;
    for(auto mtpDevice : mMtpList) {
        if(mtpDevice->mtpUmount() == PdmDevStatus::PDM_DEV_UMOUNT_FAIL)
            retVal = false;
    }
    return retVal;
}

void MTPDeviceHandler::suspendRequest() {
    for(auto mtpDevice : mMtpList)
        mtpDevice->setPowerStatus(false);
}

void MTPDeviceHandler::resumeRequest(const int &eventType) {

    for(auto mtpDevice : mMtpList)
        mtpDevice->resumeRequest(eventType);
}
