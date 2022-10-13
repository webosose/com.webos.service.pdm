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

#ifndef _HIDDEVICEHANDLER_H_
#define _HIDDEVICEHANDLER_H_

#include "CommandManager.h"
#include "DeviceHandler.h"
#include "HIDDevice.h"
#include "PdmDeviceFactory.h"
#include "PdmLogUtils.h"
#include "DeviceClass.h"

class HIDDeviceHandler : public DeviceHandler
{
private:
    const std::string iClass = ":03";
    std::list<HIDDevice*> sList;

    HIDDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    static bool mIsObjRegistered;

    //Register Object to object factory. This is called automatically
    static bool RegisterObject() {
        return (PdmDeviceFactory::getInstance()->Register("HID",
                                                          &HIDDeviceHandler::CreateObject));
    }

    void removeDevice(HIDDevice* hidDevice);
    bool m_deviceRemoved;

public:
    ~HIDDeviceHandler();

    static DeviceHandler* CreateObject(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) {
        if (mIsObjRegistered) {
            PDM_LOG_DEBUG("CreateObject - Creating the HID Device");
            return new HIDDeviceHandler(pConfObj, pluginAdapter);
        } else {
            return nullptr;
        }
    }

    bool HandlerEvent(DeviceClass*) override;
    //bool HandlerEvent(PdmNetlinkEvent* pNE) override;
    bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) override;
    bool HandlePluginEvent(int eventType) override;
    bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) override;
    bool GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message);
    void ProcessHIDDevice(DeviceClass*);
	//void ProcessHIDDevice(PdmNetlinkEvent* pNE);
    void commandNotification(EventType event, HIDDevice* device);
};

#endif // HIDDEVICEHANDLER_H
