// Copyright (c) 2019 LG Electronics, Inc.
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
#include <Common.h>
#include "PdmNetlinkEvent.h"
#include "PdmLogUtils.h"
using namespace PdmDevAttributes;

void PdmNetlinkEvent::pdmParser(struct udev_device* dev, bool isPowerOnConnect)
{
    getDeviceInfo(dev,isPowerOnConnect);

}

std::string PdmNetlinkEvent::getDevAttribute(std::string attribute)
{
    return mdeviceInfo[attribute];
}

void PdmNetlinkEvent::getDeviceInfo(struct udev_device* device, bool isPowerOnConnect)
{
    struct udev_list_entry *list_entry;

    udev_list_entry_foreach(list_entry, udev_device_get_properties_list_entry(device)){
        std::string name = udev_list_entry_get_name(list_entry);
        std::string value = udev_list_entry_get_value(list_entry);
        if(name.compare("ID_MODEL") == 0) {
            replace( value.begin(), value.end(), '_', ' ');
        }
        if(name.compare("DEVNAME") == 0){
           std::string dev = "/dev/";
           if(value.compare(0,dev.length(),dev) == 0)
              value.erase(0,dev.length());
        }
        mdeviceInfo[name] = value;
        PDM_LOG_DEBUG("PdmNetlinkEvent::getDeviceInfo - Name: %s Value: %s", name.c_str(), value.c_str());
    }
    if(isPowerOnConnect){
        mdeviceInfo[ACTION] = DEVICE_ADD;
        mdeviceInfo[IS_POWER_ON_CONNECT] = "true";
    } else {
        mdeviceInfo[IS_POWER_ON_CONNECT] = "false";
    }

}

std::string PdmNetlinkEvent::getInterfaceClass()
{
    std::string bInterfaceClass;
    if(!mdeviceInfo["ID_USB_INTERFACES"].empty())
        bInterfaceClass = mdeviceInfo["ID_USB_INTERFACES"];
    return bInterfaceClass;
}
