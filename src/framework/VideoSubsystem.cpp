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
#include "VideoSubsystem.h"
#include "DeviceClassFactory.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

bool VideoSubsystem::mIsObjRegistered = VideoSubsystem::RegisterSubSystem();

VideoSubsystem::VideoSubsystem(std::unordered_map<std::string, std::string>& devPropMap)
	: mDevType("video4linux"), DeviceClass(devPropMap)
{
	for (auto &prop : devPropMap)
		mDevPropMap[prop.first] = prop.second;
}

VideoSubsystem::~VideoSubsystem() {}

VideoSubsystem* VideoSubsystem::create(std::unordered_map<std::string, std::string>& devProMap)
{
	PDM_LOG_DEBUG("VideoSubsystem:%s line: %d", __FUNCTION__, __LINE__);
	bool canProcessEve = VideoSubsystem::canProcessEvent(devProMap);
	if(!canProcessEve)
		return nullptr;

	VideoSubsystem* ptr = new (std::nothrow) VideoSubsystem(devProMap);
	PDM_LOG_DEBUG("VideoSubsystem:%s line: %d VideoSubsystem object created", __FUNCTION__, __LINE__);
	return ptr;
}

std::string VideoSubsystem::getCapabilities()
{
	return mDevPropMap[ID_V4L_CAPABILITIES];
}

std::string VideoSubsystem::getProductName()
{
	return mDevPropMap[ID_V4L_PRODUCT];
}

std::string VideoSubsystem::getVersion()
{
	return mDevPropMap[ID_V4L_VERSION];
}

// std::string VideoSubsystem::getDevSpeed()
// {
// 	return mDevPropMap[SPEED];
// }

// std::string VideoSubsystem::getUsbDriverId()
// {
// 	return mDevPropMap[ID_USB_DRIVER];
// }