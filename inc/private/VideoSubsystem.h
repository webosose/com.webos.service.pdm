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

#ifndef _VIDEO_SUSBSYSTEM_H_
#define _VIDEO_SUSBSYSTEM_H_

#include <string>
#include <unordered_map>
#include "DeviceClass.h"
#include "DeviceClassFactory.h"

class VideoSubsystem :public DeviceClass
{
	std::string mDevType;
	std::unordered_map<std::string, std::string> mDevPropMap;
	// void init();
	static bool mIsObjRegistered;
	static bool RegisterSubSystem() {
		DeviceClassFactory::getInstance().Register("video4linux", std::bind(&VideoSubsystem::create, std::placeholders::_1));
		return true;
	}

	static bool canProcessEvent(std::unordered_map<std::string, std::string> mDevPropMap)
	{
		const std::string iClass = ":0e";
		if((mDevPropMap[PdmDevAttributes::ID_USB_INTERFACES].find(iClass) == std::string::npos) && (mDevPropMap[PdmDevAttributes::SUBSYSTEM] !=  "video4linux"))
			return false;
		return true;
	}
public:
    VideoSubsystem(std::unordered_map<std::string, std::string>& devPropMap);
	virtual ~VideoSubsystem();
	std::string getCapabilities();
	std::string getProductName();
	std::string getVersion();
	// std::string getDevSpeed();
	// std::string getUsbDriverId();
	static VideoSubsystem* create(std::unordered_map<std::string, std::string>& devPropMap);
};

#endif /* _VIDEO_SUSBSYSTEM_H_ */
