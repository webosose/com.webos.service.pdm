// Copyright (c) 2022-2024 LG Electronics, Inc.
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
#include "PdmLogUtils.h"

DeviceClassFactory::DeviceClassFactory() {}

DeviceClassFactory::~DeviceClassFactory() {}

DeviceClassFactory& DeviceClassFactory::getInstance() {
    static DeviceClassFactory obj;
	return obj;
}

void DeviceClassFactory::Register(std::string devType, devCreateFptr fPtr)
{
	mDevMap[devType] = std::move(fPtr);
}

void DeviceClassFactory::Deregister(std::string devType)
{
	auto itr = mDevMap.find(devType);
	if ( itr != mDevMap.end())
		mDevMap.erase(itr);
}

void DeviceClassFactory::parseDevProps(struct udev_device* device, bool isPowerOnConnect)
{
    mDevProMap.clear();
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
        PDM_LOG_DEBUG("DeviceClassFactory::parseDevProps - Name: %s Value: %s", name.c_str(), value.c_str());
        mDevProMap[name] = std::move(value);
    }

    if(isPowerOnConnect){
        mDevProMap[PdmDevAttributes::ACTION] = PdmDevAttributes::DEVICE_ADD;
        mDevProMap[PdmDevAttributes::IS_POWER_ON_CONNECT] = "true";
    } else {
        mDevProMap[PdmDevAttributes::IS_POWER_ON_CONNECT] = "false";
    }
}

DeviceClass* DeviceClassFactory::create(struct udev_device* device, bool isPowerOnConnect)
{
    PDM_LOG_DEBUG("DeviceClassFactory:%s line: %d mDevMap Siz: %d", __FUNCTION__, __LINE__, mDevMap.size());
    parseDevProps(device, isPowerOnConnect);
    PDM_LOG_DEBUG("DeviceClassFactory:%s line: %d mDevMap Siz: %d", __FUNCTION__, __LINE__, mDevMap.size());
    DeviceClass* subDevClasPtr;

    PDM_LOG_DEBUG("DeviceClassFactory:%s line: %d mDevMap Siz: %d", __FUNCTION__, __LINE__, mDevMap.size());

    for (auto const& dev : mDevMap) {
        subDevClasPtr = mDevMap[dev.first](mDevProMap);
        if(subDevClasPtr) {
            return subDevClasPtr;
        }
    }

    if (mDevMap.size() > 0) {
        subDevClasPtr = mDevMap["default"](mDevProMap);
        PDM_LOG_DEBUG("DeviceClassFactory:%s line: %d mDevMap Siz: %d", __FUNCTION__, __LINE__, mDevMap.size());
    }

    PDM_LOG_DEBUG("DeviceClassFactory:%s line: %d", __FUNCTION__, __LINE__);
	return subDevClasPtr;
}

