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

#include "Common.h"
#include "SoundDeviceHandler.h"
#include "PdmJson.h"

using namespace PdmDevAttributes;

bool SoundDeviceHandler::mIsObjRegistered = SoundDeviceHandler::RegisterObject();

SoundDeviceHandler::SoundDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
    : DeviceHandler(pConfObj, pluginAdapter), m_deviceRemoved(false)
{
    lunaHandler->registerLunaCallback(std::bind(&SoundDeviceHandler::GetAttachedDeviceStatus, this, _1, _2),
                                                                          GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&SoundDeviceHandler::GetAttachedAudioDeviceList, this, _1, _2),
                                                                                    GET_AUDIODEVICELIST);
    lunaHandler->registerLunaCallback(std::bind(&SoundDeviceHandler::GetAttachedNonStorageDeviceList, this, _1, _2),
                                                                                   GET_NONSTORAGEDEVICELIST);
    lunaHandler->registerLunaCallback(std::bind(&SoundDeviceHandler::GetAttachedAudioSubDeviceList, this, _1, _2),
                                                                                    GET_AUDIOSUBDEVICELIST);
}

SoundDeviceHandler::~SoundDeviceHandler() {
    if(!sList.empty())
    {
        for(auto soundDevice : sList)
        {
            delete soundDevice;
        }
        sList.clear();
    }
}

bool SoundDeviceHandler::HandlerEvent(DeviceClass* deviceClass)
{
    PDM_LOG_DEBUG("SoundDeviceHandler:%s line: %d ", __FUNCTION__, __LINE__);
    if (deviceClass->getAction() == "remove")
    {
        m_deviceRemoved = false;
        ProcessSoundDevice(deviceClass);
        if(m_deviceRemoved) {
            PDM_LOG_DEBUG("SoundDeviceHandler:%s line: %d  DEVTYPE=usb_device removed", __FUNCTION__, __LINE__);
            return true;
        }
    }

    if(!isSoundDevice(deviceClass))
        return false;

    if(deviceClass->getDevType() ==  USB_DEVICE) {
        ProcessSoundDevice(deviceClass);
        return false;
    }
    else if(deviceClass->getSubsystemName() ==  "sound") {
        ProcessSoundDevice(deviceClass);
        return true;
    }
    return false;
}

void SoundDeviceHandler::removeDevice(SoundDevice* soundDevice)
{
    if(!soundDevice)
        return;
    sList.remove(soundDevice);
    Notify(SOUND_DEVICE, REMOVE, soundDevice);
    delete soundDevice;
    soundDevice = nullptr;
}

void SoundDeviceHandler::ProcessSoundDevice(DeviceClass* deviceClass){
    SoundDevice *soundDevice;
    try {
            switch(sMapDeviceActions.at(deviceClass->getAction()))
            {
                case DeviceActions::USB_DEV_ADD:
                soundDevice = getDeviceWithPath< SoundDevice >(sList, deviceClass->getDevPath());
                if (!soundDevice) {
                    if(deviceClass->getDevName().find("timer") == std::string::npos) {
                        soundDevice = new (std::nothrow) SoundDevice(m_pConfObj, m_pluginAdapter);
                        if(soundDevice) {
                            soundDevice->setDeviceInfo(deviceClass);
                            sList.push_back(soundDevice);
                        } else {
                            PDM_LOG_CRITICAL("SoundDeviceHandler:%s line: %d Unable to create new Sound device", __FUNCTION__, __LINE__);
                        }
                    }
                } else {
                    soundDevice->updateDeviceInfo(deviceClass);
                }
                break;
                case DeviceActions::USB_DEV_REMOVE:
                soundDevice = getDeviceWithPath< SoundDevice >(sList,deviceClass->getDevPath());
                if(soundDevice) {
                   removeDevice(soundDevice);
                   m_deviceRemoved = true;
                }
                break;
             case DeviceActions::USB_DEV_CHANGE:
                // soundDevice = getDeviceWithPath< SoundDevice >(sList,pNE->getDevAttribute(DEVPATH));
                soundDevice = getDeviceWithPath< SoundDevice >(sList,deviceClass->getDevPath());
                if(soundDevice){
                   PDM_LOG_DEBUG("SoundDeviceHandlerif:%s line: %d USB_DEV_CHANGE ", __FUNCTION__, __LINE__);
                   soundDevice->updateDeviceInfo(deviceClass);
                   Notify(SOUND_DEVICE,ADD);
                }
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

bool SoundDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) {
    PDM_LOG_DEBUG("SoundDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    return false;
}

bool SoundDeviceHandler::HandlePluginEvent(int eventType)
{
    PDM_LOG_DEBUG("SoundDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    return false;
}

bool SoundDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedDeviceStatus< SoundDevice >(sList, payload );
}

bool SoundDeviceHandler::GetAttachedAudioDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedAudioDeviceList< SoundDevice >( sList, payload, false);
}

bool SoundDeviceHandler::GetAttachedAudioSubDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedAudioDeviceList< SoundDevice >( sList, payload, true);
}

bool SoundDeviceHandler::GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedNonStorageDeviceList< SoundDevice >( sList, payload );
}

bool SoundDeviceHandler::isSoundDevice(DeviceClass* deviceClass)
{
    if((deviceClass->getInterfaceClass().find(iClass) != std::string::npos) || (deviceClass->getSubsystemName() == "sound"))
        return true;

    return false;
}
