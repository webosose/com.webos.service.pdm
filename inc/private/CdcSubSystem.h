// Copyright (c) 2022 LG Electronics, Inc.
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

#ifndef _CDC_SUSBSYSTEM_H_
#define _CDC_SUSBSYSTEM_H_

#include <unordered_map>
#include <string>
#include "DeviceClass.h"
#include "DeviceClassFactory.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

class CdcSubSystem : public DeviceClass
{
    std::string mDevType;
    std::unordered_map<std::string, std::string> mDevPropMap;
    static bool mIsObjRegistered;
    static bool RegisterSubSystem()
    {
        DeviceClassFactory::getInstance().Register("cdc", std::bind(&CdcSubSystem::create, std::placeholders::_1));
        return true;
    }

    static bool canProcessEvent(std::unordered_map<std::string, std::string> mDevPropMap)
    {
        const std::string iClass = ":02";
        const std::string ethernetInterfaceClass = ":0206";
        std::string interfaceClass = mDevPropMap[ID_USB_INTERFACES];

        if ((interfaceClass.find(iClass) != std::string::npos) ||
            (mDevPropMap[ID_USB_MODEM_DONGLE] == YES) ||
            ((mDevPropMap[ID_USB_INTERFACES].find(ethernetInterfaceClass)) != std::string::npos) ||
            ((mDevPropMap[ID_USB_MODEM_DONGLE] == YES)))
            return true;

        PDM_LOG_DEBUG("CdcSubSystem:%s line: %d CdcSubSystem Object created", __FUNCTION__, __LINE__);
        return false;
    }

public:
    CdcSubSystem(std::unordered_map<std::string, std::string> &);
    virtual ~CdcSubSystem();
    static CdcSubSystem* create(std::unordered_map<std::string, std::string>&);
    std::string getUsbModemId();
    std::string getUsbSerialId();
	std::string getUsbSerialSubType();
    std::string getIdUsbSerial();
    std::string getNetIfIndex();
    std::string getNetLinkMode();
    std::string getNetDuplex();
    std::string getNetAddress();
    std::string getNetOperState();
};

#endif /* _CDC_SUSBSYSTEM_H_ */
