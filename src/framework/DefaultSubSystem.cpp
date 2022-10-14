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
#include "DefaultSubSystem.h"
#include "DeviceClassFactory.h"
#include "Common.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

bool DefaultSubSystem::mIsObjRegistered = DefaultSubSystem::RegisterSubSystem();

DefaultSubSystem::DefaultSubSystem(std::unordered_map<std::string, std::string>& devPropMap)
	: mDevType("default"), DeviceClass(devPropMap)
{
	for (auto &prop : devPropMap)
		mDevPropMap[prop.first] = prop.second;
}

DefaultSubSystem::~DefaultSubSystem() {}

DefaultSubSystem* DefaultSubSystem::create(std::unordered_map<std::string, std::string>& devProMap)
{
	PDM_LOG_DEBUG("DefaultSubSystem:%s line: %d", __FUNCTION__, __LINE__);

	DefaultSubSystem* ptr = new (std::nothrow) DefaultSubSystem(devProMap);
	PDM_LOG_DEBUG("DefaultSubSystem:%s line: %d DefaultSubSystem object created", __FUNCTION__, __LINE__);
	return ptr;
}