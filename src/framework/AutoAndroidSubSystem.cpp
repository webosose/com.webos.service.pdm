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
#include "AutoAndroidSubSystem.h"
#include "DeviceClassFactory.h"
#include "Common.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

bool AutoAndroidSubSystem::mIsObjRegistered = AutoAndroidSubSystem::RegisterSubSystem();

AutoAndroidSubSystem::AutoAndroidSubSystem(std::unordered_map<std::string, std::string>& devPropMap)
	: mDevType("autoandroid"), DeviceClass(devPropMap)
{
	for (auto &prop : devPropMap)
		mDevPropMap[prop.first] = prop.second;
}

AutoAndroidSubSystem::~AutoAndroidSubSystem() {}

AutoAndroidSubSystem *AutoAndroidSubSystem::create(std::unordered_map<std::string, std::string> &devProMap)
{
    PDM_LOG_DEBUG("AutoAndroidSubSystem:%s line: %d", __FUNCTION__, __LINE__);
    bool canProcessEve = AutoAndroidSubSystem::canProcessEvent(devProMap);

    if (!canProcessEve)
        return nullptr;

    AutoAndroidSubSystem *ptr = new (std::nothrow) AutoAndroidSubSystem(devProMap);
    PDM_LOG_DEBUG("AutoAndroidSubSystem:%s line: %d AutoAndroidSubSystem object created", __FUNCTION__, __LINE__);
    return ptr;
}

std::string AutoAndroidSubSystem::getIdProduct()
{
    return mDevPropMap[ID_PRODUCT_ID];
}

std::string AutoAndroidSubSystem::getModelId()
{
    return mDevPropMap[ID_MODEL_ID];
}