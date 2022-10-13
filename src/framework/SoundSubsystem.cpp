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
#include "SoundSubsystem.h"
#include "DeviceClassFactory.h"
#include "Common.h"

using namespace PdmDevAttributes;

SoundSubsystem::SoundSubsystem(std::unordered_map<std::string, std::string>& devPropMap)
	: mDevType("Sound_Dev"), /*ToDo*/ DeviceClass(devPropMap)
{
	for (auto &prop : devPropMap)
		mDevPropMap[prop.first] = prop.second;
}

SoundSubsystem::~SoundSubsystem() {}

void SoundSubsystem::init()
{
	// ToDo : init method needs to be removed and Register call should be made from correct place
	DeviceClassFactory::getInstance().Register(mDevType,
		std::bind(&SoundSubsystem::create, std::placeholders::_1));
}

SoundSubsystem* SoundSubsystem::create(std::unordered_map<std::string, std::string>& devProMap)
{
	SoundSubsystem* ptr = new (std::nothrow) SoundSubsystem(devProMap);
	return ptr;
}

std::string SoundSubsystem::getDevType()
{
	return mDevPropMap[DEVTYPE];
}

std::string SoundSubsystem::getDevSpeed()
{
	return mDevPropMap[SPEED];
}

std::string SoundSubsystem::getCardId()
{
	return mDevPropMap[CARD_ID];
}

std::string SoundSubsystem::getCardName()
{
	return mDevPropMap[CARD_NAME];
}

std::string SoundSubsystem::getCardNumber()
{
	return mDevPropMap[CARD_NUMBER];
}