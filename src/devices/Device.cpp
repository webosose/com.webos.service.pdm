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

#include <cstddef>

#include "Device.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

Device::Device(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter, std::string deviceType, std::string errorReason):
    m_isPowerOnConnect(false), m_isToastRequired(true), m_deviceNum(0), m_usbPortNum(0), m_busNum(0), m_usbPortSpeed(0), m_pConfObj(pConfObj)
    , m_pluginAdapter(pluginAdapter), m_deviceStatus(PdmDevAttributes::PDM_ERR_NOTHING), m_deviceType(deviceType), m_errorReason(errorReason), m_deviceName("")
    , m_devicePath(""), m_serialNumber(""), m_vendorName(""), m_productName(""), m_devSpeed(""), m_deviceSubType("")
{
}

void Device::setDeviceInfo(PdmNetlinkEvent* pNE)
{
    m_serialNumber = pNE->getDevAttribute(ID_SERIAL_SHORT);
    m_deviceSubType = pNE->getDevAttribute(ID_USB_DRIVER);
    m_productName = pNE->getDevAttribute(ID_MODEL);

    if (!m_pluginAdapter->getPowerState() || pNE->getDevAttribute(IS_POWER_ON_CONNECT) == "true")
      m_isPowerOnConnect = true;

    if(pNE->getDevAttribute(DEVTYPE) == USB_DEVICE){
      m_devicePath = pNE->getDevAttribute(DEVPATH);
    }
    if(!pNE->getDevAttribute(ID_VENDOR_FROM_DATABASE).empty()){
        m_vendorName = pNE->getDevAttribute(ID_VENDOR_FROM_DATABASE);
    } else {
        m_vendorName = pNE->getDevAttribute(ID_VENDOR);
    }
    if(!pNE->getDevAttribute(DEVNUM).empty())
        m_deviceNum = std::stoi(pNE->getDevAttribute(DEVNUM),nullptr);
}

std::string  Device::getDeviceSpeed(int speed) const {

    switch(speed){
        case SUPER:
            return USB_SUPER_SPEED;
        case HIGH:
            return USB_HIGH_SPEED;
        case FULL:
            return USB_FULL_SPEED;
        default:
            return USB_LOW_SPEED;
    }
}
