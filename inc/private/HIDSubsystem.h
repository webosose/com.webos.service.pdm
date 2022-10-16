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

#ifndef _HID_SUSBSYSTEM_H_
#define _HID_SUSBSYSTEM_H_

#include <string>
#include <unordered_map>
#include "DeviceClass.h"
#include "DeviceClassFactory.h"
#include "PdmLogUtils.h"

class HIDSubsystem :public DeviceClass
{
	std::string mDevType;
	std::unordered_map<std::string, std::string> mDevPropMap;
	static bool mIsObjRegistered;
	static bool RegisterSubSystem() {
		PDM_LOG_DEBUG("HIDSubsystem:%s line: %d HIDSubsystem Registered", __FUNCTION__, __LINE__);
		DeviceClassFactory::getInstance().Register("input", std::bind(&HIDSubsystem::create, std::placeholders::_1));
		return true;
	}
    static bool canProcessEvent(std::unordered_map<std::string, std::string> mDevPropMap)
    {
        const std::string iClass = ":03";
        if (mDevPropMap[PdmDevAttributes::ID_USB_INTERFACES].find(iClass) != std::string::npos)
            return true;
        PDM_LOG_DEBUG("HIDSubsystem:%s line: %d HIDSubsystem Object created", __FUNCTION__, __LINE__);
        return false;
    }

public:
    HIDSubsystem(std::unordered_map<std::string, std::string>& devPropMap);
    virtual ~HIDSubsystem();
	static HIDSubsystem* create(std::unordered_map<std::string, std::string>& devPropMap);
	std::string getProcessed();
};

#endif /* _HID_SUSBSYSTEM_H_ */
