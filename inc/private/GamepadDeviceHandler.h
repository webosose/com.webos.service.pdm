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

#ifndef _GAMEPADDEVICEHANDLER_H_
#define _GAMEPADDEVICEHANDLER_H_

#include "CommandManager.h"
#include "DeviceHandler.h"
#include "GamepadDevice.h"
#include "PdmDeviceFactory.h"
#include "PdmLogUtils.h"
#include "PdmNetlinkEvent.h"

class GamepadDeviceHandler : public DeviceHandler
{
private:
    std::list<GamepadDevice*> sList;
    static bool mIsObjRegistered;
    GamepadDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    //Register Object to object factory. This is called automatically
    static bool RegisterObject() {
        return (PdmDeviceFactory::getInstance()->Register("GAMEPAD",
                                                          &GamepadDeviceHandler::CreateObject));
    }
    void removeDevice(GamepadDevice* hdl);
    bool m_deviceRemoved;

public:
    ~GamepadDeviceHandler() = default;
    static DeviceHandler* CreateObject(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) {
        if (mIsObjRegistered) {
            PDM_LOG_DEBUG("CreateObject - Creating the Gamepad device handler");
            return new GamepadDeviceHandler(pConfObj,pluginAdapter);
        } else {
            return nullptr;
        }
    }
    bool HandlerEvent(PdmNetlinkEvent* pNE) override;
    bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) override;
    bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) override;
    bool GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message);
    void ProcessGamepadDevice(PdmNetlinkEvent* pNE);
};

#endif // GAMEPADDEVICEHANDLER_H

