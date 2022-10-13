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

#include <algorithm>
#include "DeviceClassFactory.h"
#include "Common.h"

DeviceClassFactory::DeviceClassFactory() {}

DeviceClassFactory::~DeviceClassFactory() {}

DeviceClassFactory& DeviceClassFactory::getInstance() {
    static DeviceClassFactory obj;
	return obj;
}

void DeviceClassFactory::Register(std::string devType, devCreateFptr fPtr)
{
	mDevMap[devType] = fPtr;
}

void DeviceClassFactory::Deregister(std::string devType)
{
	auto itr = mDevMap.find(devType);
	if ( itr != mDevMap.end())
		mDevMap.erase(itr);
}

void DeviceClassFactory::parseDevProps(struct udev_device* device)
{
    struct udev_list_entry *list_entry;
    udev_list_entry_foreach(list_entry, udev_device_get_properties_list_entry(device)){
        std::string name = udev_list_entry_get_name(list_entry);
        std::string value = udev_list_entry_get_value(list_entry);
        if(name.compare("ID_MODEL") == 0) {
            std::replace( value.begin(), value.end(), '_', ' ');
        }
        if(name.compare("DEVNAME") == 0){
           std::string dev = "/dev/";
           if(value.compare(0,dev.length(),dev) == 0)
              value.erase(0,dev.length());
        }
		mDevProMap[name] = value;
    }
}

DeviceClass* DeviceClassFactory::create(struct udev_device* device)
{
	parseDevProps(device);
	std::string devType = mDevProMap[PdmDevAttributes::DEVTYPE];
	DeviceClass* devClasPtr = NULL;
	if (mDevMap.count(devType) != 0)
		devClasPtr = mDevMap[devType](mDevProMap);
	return devClasPtr;
}

