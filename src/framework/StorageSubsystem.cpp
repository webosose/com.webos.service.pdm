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

#include <functional>
#include "Common.h"
#include "StorageSubsystem.h"
#include "DeviceClassFactory.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

bool StorageSubsystem::mIsObjRegistered = StorageSubsystem::RegisterSubSystem();

StorageSubsystem::StorageSubsystem(std::unordered_map<std::string, std::string>& devPropMap)
	: mDevType("storage"), DeviceClass(devPropMap)
{
	for (auto &prop : devPropMap)
		mDevPropMap[prop.first] = prop.second;
}

StorageSubsystem::~StorageSubsystem() {}

StorageSubsystem *StorageSubsystem::create(std::unordered_map<std::string, std::string> &devProMap)
{
    bool canProcessEve = StorageSubsystem::canProcessEvent(devProMap);

    if (!canProcessEve)
        return nullptr;

    StorageSubsystem *ptr = new (std::nothrow) StorageSubsystem(devProMap);
    PDM_LOG_DEBUG("StorageSubsystem:%s line: %d StorageSubsystem Object created", __FUNCTION__, __LINE__);
    return ptr;
}

std::string StorageSubsystem::getHardDisk()
{
    return mDevPropMap[ID_ATA];
}

std::string StorageSubsystem::isCardReader()
{
    return mDevPropMap[CARD_READER];
}

std::string StorageSubsystem::isHardDisk()
{
    return mDevPropMap[HARD_DISK];
}

std::string StorageSubsystem::getIdInstance()
{
    return mDevPropMap[ID_INSTANCE];
}

std::string StorageSubsystem::getIdBlackListedSuperSpeedDev()
{
    return mDevPropMap[ID_BLACK_LISTED_SUPER_SPEED_DEVICE];
}

std::string StorageSubsystem::isReadOnly()
{
    return mDevPropMap[READ_ONLY];
}

std::string StorageSubsystem::isDiskMediaChange()
{
    return mDevPropMap[DISK_MEDIA_CHANGE];
}