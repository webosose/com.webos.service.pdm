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

#include "CdcDevice.h"
#include "Common.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

//USB2SERIAL
const std::string USB2SERIAL = "USB2SERIAL";
//const std::string USB_SERIAL_SUB_TYPE = "USB_SERIAL_SUB_TYPE";
//Modem dongle
const std::string MODEM_DONGLE = "USBMODEM";

CdcDevice::CdcDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
                : Device(pConfObj, pluginAdapter, "CDC", PDM_ERR_NOTHING)
                , m_ifindex("")
                , m_linkMode("")
                , m_duplex("")
                , m_address("")
                , m_operstate(""){
}


void CdcDevice::setDeviceInfo(DeviceClass* devClass)
{
    PDM_LOG_DEBUG("CdcDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if( (devClass->getDevType() == USB_DEVICE) || (devClass->getUsbModemId() == YES) ) {
        if(!devClass->getSpeed().empty())
            m_devSpeed = getDeviceSpeed(stoi(devClass->getSpeed(), nullptr));
        Device::setDeviceInfo(devClass);
        if ("1" == devClass->getUsbSerialId()){
            PDM_LOG_DEBUG("CdcDevice:%s line: %d It is USB to serial device", __FUNCTION__, __LINE__);
            m_deviceType = USB2SERIAL;
            m_deviceSubType = devClass->getUsbSerialSubType();
        }
        if((devClass->getUsbModemId() == YES))
        {
            PDM_LOG_DEBUG("CdcDevice:%s line: %d It is modem dongle device", __FUNCTION__, __LINE__);
            m_deviceType = MODEM_DONGLE;
            m_devSpeed = USB_HIGH_SPEED;
        }
    }
}

#if 0
void CdcDevice::setDeviceInfo(PdmNetlinkEvent* pNE)
{
    PDM_LOG_DEBUG("CdcDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if( (pNE->getDevAttribute(DEVTYPE) == USB_DEVICE) || (pNE->getDevAttribute(ID_USB_MODEM_DONGLE) == YES) ) {
        if(!pNE->getDevAttribute(SPEED).empty())
            m_devSpeed = getDeviceSpeed(stoi(pNE->getDevAttribute(SPEED),nullptr));
        Device::setDeviceInfo(pNE);
        if ("1" == pNE->getDevAttribute(ID_USB_SERIAL)){
            PDM_LOG_DEBUG("CdcDevice:%s line: %d It is USB to serial device", __FUNCTION__, __LINE__);
            m_deviceType = USB2SERIAL;
            m_deviceSubType = pNE->getDevAttribute(USB_SERIAL_SUB_TYPE);
        }
        if((pNE->getDevAttribute(ID_USB_MODEM_DONGLE) == YES))
        {
            PDM_LOG_DEBUG("CdcDevice:%s line: %d It is modem dongle device", __FUNCTION__, __LINE__);
            m_deviceType = MODEM_DONGLE;
            m_devSpeed = USB_HIGH_SPEED;
        }
    }
}
#endif

void CdcDevice::updateDeviceInfo(DeviceClass* devClass)
{
    if((devClass->getSubsystemName()== "net") && (m_deviceType != MODEM_DONGLE)){
        PDM_LOG_DEBUG("CdcDevice:%s line: %d It is Net device", __FUNCTION__, __LINE__);
        if(!(devClass->getUsbDriver().empty()))
            m_deviceSubType = devClass->getUsbDriver();

        if(!(devClass->getNetIfIndex().empty()))
            m_ifindex = devClass->getNetIfIndex();

        if(!(devClass->getNetLinkMode().empty()))
            m_linkMode = devClass->getNetLinkMode();

        if(!(devClass->getNetDuplex().empty()))
            m_duplex = devClass->getNetDuplex();

        if(!(devClass->getNetAddress().empty()))
            m_address = devClass->getNetAddress();

        if(!(devClass->getNetOperState().empty()))
            m_operstate = devClass->getNetOperState();

         m_isToastRequired = false;
    }
}

#if 0
void CdcDevice::updateDeviceInfo(PdmNetlinkEvent* pNE)
{
    if((pNE->getDevAttribute(SUBSYSTEM) == "net") && (m_deviceType != MODEM_DONGLE)){
        PDM_LOG_DEBUG("CdcDevice:%s line: %d It is Net device", __FUNCTION__, __LINE__);
        if(!(pNE->getDevAttribute(ID_USB_DRIVER).empty()))
            m_deviceSubType = pNE->getDevAttribute(ID_USB_DRIVER);

        if(!(pNE->getDevAttribute(NET_IFIINDEX).empty()))
            m_ifindex = pNE->getDevAttribute(NET_IFIINDEX);

        if(!(pNE->getDevAttribute(NET_LINK_MODE).empty()))
            m_linkMode = pNE->getDevAttribute(NET_LINK_MODE);

        if(!(pNE->getDevAttribute(NET_DUPLEX).empty()))
            m_duplex = pNE->getDevAttribute(NET_DUPLEX);

        if(!(pNE->getDevAttribute(NET_ADDRESS).empty()))
            m_address = pNE->getDevAttribute(NET_ADDRESS);

        if(!(pNE->getDevAttribute(NET_OPERSTATE).empty()))
            m_operstate = pNE->getDevAttribute(NET_OPERSTATE);

         m_isToastRequired = false;
    }
}
#endif

