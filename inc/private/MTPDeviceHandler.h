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

#ifndef MTPDEVICEHANDLER_H
#define MTPDEVICEHANDLER_H

#include "CommandManager.h"
#include "DeviceHandler.h"
#include "MTPDevice.h"
#include "PdmDeviceFactory.h"
#include "PdmLogUtils.h"
#include "DeviceClass.h"

class MTPDeviceHandler : public DeviceHandler
{
private:
    static bool mIsObjRegistered;
    std::list<MTPDevice*> mMtpList;
    MTPDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    //Register Object to object factory. This is called automatically
    static bool RegisterObject() {
        return (PdmDeviceFactory::getInstance()->Register("MTP",
                                                  &MTPDeviceHandler::CreateObject));
    }
    void removeDevice(MTPDevice* mtpDevice);
    bool eject(CommandType *cmdtypes, CommandResponse *cmdResponse);
    bool umountAllDrive();
    void suspendRequest();
    void resumeRequest(const int &eventType);

public:
    ~MTPDeviceHandler();
    static DeviceHandler* CreateObject(PdmConfig* const pConfObj,
                                               PluginAdapter* const pluginAdapter) {
        if (mIsObjRegistered)
            return new MTPDeviceHandler(pConfObj, pluginAdapter);
        return nullptr;
    }
    bool HandlerEvent(DeviceClass* deviceClass) override;
    //bool HandlerEvent(PdmNetlinkEvent* pNE) override;
    bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) override;
    bool HandlePluginEvent(int eventType) override;
    bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) override;
    bool GetAttachedStorageDeviceList (pbnjson::JValue &payload, LSMessage *message);
    void ProcessMTPDevice(DeviceClass*);
	//void ProcessMTPDevice(PdmNetlinkEvent* pNE);
    void commandNotification(EventType event, MTPDevice* device);
};
#endif //MTPDEVICEHANDLER_H
