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

#ifndef _GAMEPAD_SUSBSYSTEM_H_
#define _GAMEPAD_SUSBSYSTEM_H_

#include <unordered_map>
#include <string>
#include "DeviceClass.h"
#include "DeviceClassFactory.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

class GamepadSubSystem : public DeviceClass
{
    std::string mDevType;
    std::unordered_map<std::string, std::string> mDevPropMap;
    static bool mIsObjRegistered;
    static bool RegisterSubSystem()
    {
        DeviceClassFactory::getInstance().Register("gamepad", std::bind(&GamepadSubSystem::create, std::placeholders::_1));
        return true;
    }

    static bool canProcessEvent(std::unordered_map<std::string, std::string> mDevPropMap)
    {
        if(mDevPropMap[ID_GAMEPAD] == "1")
            return true;

        PDM_LOG_DEBUG("GamepadSubSystem:%s line: %d GamepadSubSystem Object created", __FUNCTION__, __LINE__);
        return false;
    }

public:
    GamepadSubSystem(std::unordered_map<std::string, std::string> &);
    virtual ~GamepadSubSystem();
    static GamepadSubSystem* create(std::unordered_map<std::string, std::string>&);
    std::string getGamepadId();
};

#endif /* _GAMEPAD_SUSBSYSTEM_H_ */
