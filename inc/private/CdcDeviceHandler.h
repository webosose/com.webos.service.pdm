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

#ifndef _CDCDEVICEHANDLER_H_
#define _CDCDEVICEHANDLER_H_

#include "CdcDevice.h"
#include "Common.h"
#include "DeviceHandler.h"
#include "PdmDeviceFactory.h"
#include "PdmLogUtils.h"
#include "PdmNetlinkEvent.h"

class CdcDeviceHandler : public DeviceHandler
{
private:
    bool m_is3g4gDongleSupported;
    const std::string iClass = ":02";
    const std::string ethernetInterfaceClass = ":0206";
    std::list<CdcDevice*> sList;

    CdcDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    static bool mIsObjRegistered;

    //Register Object to object factory. This is called automatically
    static bool RegisterObject() {
        return (PdmDeviceFactory::getInstance()->Register("CDC",
                                                          &CdcDeviceHandler::CreateObject));
    }

    void removeDevice(CdcDevice* hdl);

public:
    ~CdcDeviceHandler();

    static DeviceHandler* CreateObject(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) {
        if (mIsObjRegistered) {
            PDM_LOG_DEBUG("CreateObject - Creating the CDC device handler");
            return new CdcDeviceHandler(pConfObj, pluginAdapter);
        }
        return nullptr;
    }

    bool HandlerEvent(PdmNetlinkEvent* pNE) override;
    bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) override;
    bool HandlePluginEvent(int eventType) override;
    bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) override;
    bool GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message);
    void ProcessCdcDevice(PdmNetlinkEvent* pNE);
    bool identifyCdcDevice(PdmNetlinkEvent* pNE);
    bool GetAttachedNetDeviceList(pbnjson::JValue &payload, LSMessage *message);
};

#endif // CDCDEVICEHANDLER_H
