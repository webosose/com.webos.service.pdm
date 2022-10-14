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

#include "GamepadDeviceHandler.h"
#include "PdmJson.h"
#include "GamepadSubSystem.h"

using namespace PdmDevAttributes;

bool GamepadDeviceHandler::mIsObjRegistered = GamepadDeviceHandler::RegisterObject();

GamepadDeviceHandler::GamepadDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
                     : DeviceHandler(pConfObj, pluginAdapter), m_deviceRemoved(false) {
    lunaHandler->registerLunaCallback(std::bind(&GamepadDeviceHandler::GetAttachedDeviceStatus, this, _1, _2),
                                      GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&GamepadDeviceHandler::GetAttachedNonStorageDeviceList, this, _1, _2),
                                      GET_NONSTORAGEDEVICELIST);
}

bool GamepadDeviceHandler::HandlerEvent(DeviceClass* devClass)
{
    GamepadSubSystem* gamepadSubsystem = (GamepadSubSystem*)devClass;
	if (gamepadSubsystem == nullptr) return false;

    if (gamepadSubsystem->getAction() == "remove")
    {
      m_deviceRemoved = false;
      ProcessGamepadDevice(gamepadSubsystem);
      if(m_deviceRemoved) {
          PDM_LOG_DEBUG("GamepadDeviceHandler:%s line: %d  DEVTYPE=usb_device removed", __FUNCTION__, __LINE__);
          return true;
      }
    }
    if(gamepadSubsystem->getGamepadId() == "1"){
        ProcessGamepadDevice(gamepadSubsystem);
        return true;
    }
    return false;
}

#if 0
bool GamepadDeviceHandler::HandlerEvent(PdmNetlinkEvent* pNE){

    if (pNE->getDevAttribute(ACTION) == "remove")
    {
      m_deviceRemoved = false;
      ProcessGamepadDevice(pNE);
      if(m_deviceRemoved) {
          PDM_LOG_DEBUG("GamepadDeviceHandler:%s line: %d  DEVTYPE=usb_device removed", __FUNCTION__, __LINE__);
          return true;
      }
    }
    if(pNE->getDevAttribute(ID_GAMEPAD) == "1"){
        ProcessGamepadDevice(pNE);
        return true;
    }
    return false;
}
#endif

void GamepadDeviceHandler::removeDevice(GamepadDevice* gamePadDevice)
{
    sList.remove(gamePadDevice);
    Notify(GAMEPAD_DEVICE, REMOVE, gamePadDevice);
    delete gamePadDevice;
    gamePadDevice = nullptr;
}

void GamepadDeviceHandler::ProcessGamepadDevice(DeviceClass* devClass){
    GamepadDevice *gamepadDevice;
    PDM_LOG_INFO("GamepadDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__, __LINE__, devClass->getDevType().c_str(), devClass->getAction().c_str());

    try {
            switch(sMapDeviceActions.at(devClass->getAction()))
            {
                case DeviceActions::USB_DEV_ADD:
                    PDM_LOG_DEBUG("GamepadDeviceHandler:%s line: %d action : %s", __FUNCTION__, __LINE__, devClass->getAction().c_str());
                    gamepadDevice = new (std::nothrow) GamepadDevice(m_pConfObj, m_pluginAdapter);
                    if(gamepadDevice) {
                        gamepadDevice->setDeviceInfo(devClass);
                        sList.push_back(gamepadDevice);
                        Notify(GAMEPAD_DEVICE,ADD);
                    }
                    break;
                case DeviceActions::USB_DEV_REMOVE:
                   PDM_LOG_DEBUG("GamepadDeviceHandler:%s line: %d action : %s", __FUNCTION__, __LINE__, devClass->getAction().c_str());
                   gamepadDevice = getDeviceWithPath< GamepadDevice >(sList, devClass->getDevPath());
                   if(gamepadDevice) {
                      removeDevice(gamepadDevice);
                      m_deviceRemoved = true;
                   }
                     break;
                default:
                 //Do nothing
                 break;
        }
    }
      catch (const std::out_of_range& err) {
         PDM_LOG_INFO("GamepadDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }
}

#if 0
void GamepadDeviceHandler::ProcessGamepadDevice(PdmNetlinkEvent* pNE){
    GamepadDevice *gamepadDevice;
    PDM_LOG_INFO("GamepadDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__,__LINE__,pNE->getDevAttribute(DEVTYPE).c_str(),pNE->getDevAttribute(ACTION).c_str());

    try {
            switch(sMapDeviceActions.at(pNE->getDevAttribute(ACTION)))
            {
                case DeviceActions::USB_DEV_ADD:
                    PDM_LOG_DEBUG("GamepadDeviceHandler:%s line: %d action : %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(ACTION).c_str());
                    gamepadDevice = new (std::nothrow) GamepadDevice(m_pConfObj, m_pluginAdapter);
                    if(gamepadDevice) {
                        gamepadDevice->setDeviceInfo(pNE);
                        sList.push_back(gamepadDevice);
                        Notify(GAMEPAD_DEVICE,ADD);
                    }
                    break;
                case DeviceActions::USB_DEV_REMOVE:
                   PDM_LOG_DEBUG("GamepadDeviceHandler:%s line: %d action : %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(ACTION).c_str());
                   gamepadDevice = getDeviceWithPath< GamepadDevice >(sList,pNE->getDevAttribute(DEVPATH));
                   if(gamepadDevice) {
                      removeDevice(gamepadDevice);
                      m_deviceRemoved = true;
                   }
                     break;
                default:
                 //Do nothing
                 break;
        }
    }
      catch (const std::out_of_range& err) {
         PDM_LOG_INFO("GamepadDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }
}
#endif

bool GamepadDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) {

    PDM_LOG_DEBUG("GamepadDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    return false;
}

bool GamepadDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedDeviceStatus< GamepadDevice >(sList, payload );
}

bool GamepadDeviceHandler::GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedNonStorageDeviceList< GamepadDevice >( sList, payload );
}
