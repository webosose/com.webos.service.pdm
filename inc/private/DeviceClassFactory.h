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

#ifndef DEVICECLASSFACTORY_H_
#define DEVICECLASSFACTORY_H_

#include <libudev.h>

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include "DeviceClass.h"

using devCreateFptr =  std::function<DeviceClass* (std::unordered_map<std::string, std::string>&)>;
using devCreatorMap = std::unordered_map<std::string, devCreateFptr>;

class DeviceClassFactory {
private:
	devCreatorMap mDevMap;
	std::unordered_map<std::string, std::string> mDevProMap;
	void parseDevProps(struct udev_device*);
	explicit DeviceClassFactory();
	DeviceClassFactory(const DeviceClassFactory&) = delete;
    DeviceClassFactory& operator=(const DeviceClassFactory&) = delete;
    DeviceClassFactory(DeviceClassFactory&&) = delete;
    DeviceClassFactory& operator=(DeviceClassFactory&&) = delete;
public:
	static DeviceClassFactory& getInstance();
	void Register(std::string, devCreateFptr);
	void Deregister(std::string);
	DeviceClass* create(struct udev_device*);
	~DeviceClassFactory();
};

#endif /* DEVICECLASSFACTORY_H_ */
