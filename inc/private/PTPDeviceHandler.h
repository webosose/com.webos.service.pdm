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

#ifndef PTPDEVICEHANDLER_H
#define PTPDEVICEHANDLER_H

#include "CommandManager.h"
#include "DeviceHandler.h"
#include "PTPDevice.h"
#include "PdmDeviceFactory.h"
#include "PdmNetlinkEvent.h"
#include "PdmLogUtils.h"

class PTPDeviceHandler : public DeviceHandler
{
private:
    const std::string iClass = ":06";
    static bool mIsObjRegistered;
    std::list<PTPDevice*> sList;
    PTPDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    //Register Object to object factory. This is called automatically
    static bool RegisterObject() {
        return (PdmDeviceFactory::getInstance()->Register("PTP",
                                                   &PTPDeviceHandler::CreateObject));
    }
    std::string deviceName(const char* sysName, int hostID);
    void removeDevice(PTPDevice* ptpDevice);
    bool eject(CommandType *cmdtypes, CommandResponse *cmdResponse);
    bool umountAllDrive();
    void suspendRequest();
    void resumeRequest(const int &eventType);
    void commandNotification(EventType event, PTPDevice* device);

public:
    ~PTPDeviceHandler();
    static DeviceHandler* CreateObject(PdmConfig* const pConfObj,
                                               PluginAdapter* const pluginAdapter) {
        if (mIsObjRegistered)
            return new PTPDeviceHandler(pConfObj, pluginAdapter);
        return nullptr;
    }
    bool HandlerEvent(PdmNetlinkEvent* pNE) override;
    bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) override;
    bool HandlePluginEvent(int eventType) override;
    bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) override;
    bool GetAttachedStorageDeviceList (pbnjson::JValue &payload, LSMessage *message);
    void ProcessPTPDevice(PdmNetlinkEvent* pNE);
};
#endif // PTPDEVICEHANDLER_H
