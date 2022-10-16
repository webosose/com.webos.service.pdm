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
#include "PTPSubsystem.h"
#include "DeviceClassFactory.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

bool PTPSubsystem::mIsObjRegistered = PTPSubsystem::RegisterSubSystem();

PTPSubsystem::PTPSubsystem(std::unordered_map<std::string, std::string> &devPropMap)
    : mDevType("ptp"), DeviceClass(devPropMap)
{
    for (auto &prop : devPropMap)
        mDevPropMap[prop.first] = prop.second;
}

PTPSubsystem::~PTPSubsystem() {}

PTPSubsystem *PTPSubsystem::create(std::unordered_map<std::string, std::string> &devProMap)
{
    bool canProcessEve = PTPSubsystem::canProcessEvent(devProMap);

    if (!canProcessEve)
        return nullptr;

    PTPSubsystem *ptr = new (std::nothrow) PTPSubsystem(devProMap);
    PDM_LOG_DEBUG("PTPSubsystem:%s line: %d PTPSubsystem Object created", __FUNCTION__, __LINE__);
    return ptr;
}

std::string PTPSubsystem::getBusNum()
{
	return mDevPropMap[BUSNUM];
}