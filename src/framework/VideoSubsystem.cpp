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

VideoSubsystem::VideoSubsystem(std::unordered_map<std::string, std::string>& devPropMap)
	: mDevType("video4linux")  /*ToDo*/, DeviceClass(devPropMap)
{
	for (auto &prop : devPropMap)
		mDevPropMap[prop.first] = prop.second;
}

VideoSubsystem::~VideoSubsystem() {}

void VideoSubsystem::init()
{
	// ToDo : init method needs to be removed and Register call should be made from correct place
	DeviceClassFactory::getInstance().Register(mDevType,
		std::bind(&VideoSubsystem::create, std::placeholders::_1));
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

std::string VideoSubsystem::getDevSpeed()
{
	return mDevPropMap[PdmDevAttributes::SPEED];
}

std::string VideoSubsystem::getUsbDriverId()
{
	return mDevPropMap[PdmDevAttributes::ID_USB_DRIVER];
}

VideoSubsystem* VideoSubsystem::create(std::unordered_map<std::string, std::string>& devProMap)
{
	VideoSubsystem* ptr = new (std::nothrow) VideoSubsystem(devProMap);
	return ptr;
}

