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
#include "HIDSubsystem.h"
#include "DeviceClassFactory.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;
bool HIDSubsystem::mIsObjRegistered = HIDSubsystem::RegisterSubSystem();

HIDSubsystem::HIDSubsystem(std::unordered_map<std::string, std::string>& devPropMap)
	: mDevType("input"), DeviceClass(devPropMap)
{
	for (auto &prop : devPropMap)
		mDevPropMap[prop.first] = prop.second;
}

HIDSubsystem::~HIDSubsystem() {}

HIDSubsystem *HIDSubsystem::create(std::unordered_map<std::string, std::string> &devProMap)
{
    bool canProcessEve = HIDSubsystem::canProcessEvent(devProMap);

    if (!canProcessEve)
        return nullptr;

    HIDSubsystem *ptr = new (std::nothrow) HIDSubsystem(devProMap);
    PDM_LOG_DEBUG("HIDSubsystem:%s line: %d HIDSubsystem Object created", __FUNCTION__, __LINE__);
    return ptr;
}

std::string HIDSubsystem::getProcessed()
{
    return mDevPropMap[PROCESSED];
}
