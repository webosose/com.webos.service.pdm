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

#ifndef _STORAGE_SUSBSYSTEM_H_
#define _STORAGE_SUSBSYSTEM_H_

#include <string>
#include <unordered_map>
#include "DeviceClass.h"
#include "DeviceClassFactory.h"
#include "PdmLogUtils.h"

#define PDM_HDD_ID_ATA "1"

using namespace PdmDevAttributes;

class StorageSubsystem : public DeviceClass
{
    std::string mDevType;
    std::unordered_map<std::string, std::string> mDevPropMap;
    static bool mIsObjRegistered;
    static bool RegisterSubSystem()
    {
        PDM_LOG_DEBUG("StorageSubsystem:%s line: %d StorageSubsystem Registered", __FUNCTION__, __LINE__);
        DeviceClassFactory::getInstance().Register("storage", std::bind(&StorageSubsystem::create, std::placeholders::_1));
        return true;
    }
    static bool canProcessEvent(std::unordered_map<std::string, std::string> mDevPropMap)
    {
        const std::string iClass = ":08";
        std::string interfaceClass = mDevPropMap[ID_USB_INTERFACES];
        if ((interfaceClass.find(iClass) != std::string::npos) || (mDevPropMap[ID_ATA] == PDM_HDD_ID_ATA))
            return true;
        PDM_LOG_DEBUG("StorageSubsystem:%s line: %d StorageSubsystem Object created", __FUNCTION__, __LINE__);
        return false;
    }

public:
    StorageSubsystem(std::unordered_map<std::string, std::string> &devPropMap);
    virtual ~StorageSubsystem();
    static StorageSubsystem *create(std::unordered_map<std::string, std::string> &devPropMap);
    std::string getHardDisk();
    std::string isCardReader();
    std::string isHardDisk();
    std::string getIdInstance();
    std::string getIdBlackListedSuperSpeedDev();
    std::string isReadOnly();
    std::string isDiskMediaChange();
};

#endif /* _STORAGE_SUSBSYSTEM_H_ */
