// Copyright (c) 2019 -2022 LG Electronics, Inc.
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
#include "CdcSubSystem.h"

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
    CdcSubSystem *cdcSubsystem = (CdcSubSystem *)devClass;

    PDM_LOG_DEBUG("CdcDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if( (devClass->getDevType() == USB_DEVICE) || (cdcSubsystem->getUsbModemId() == YES) ) {
        if(!devClass->getSpeed().empty())
            m_devSpeed = getDeviceSpeed(stoi(devClass->getSpeed(), nullptr));
        Device::setDeviceInfo(devClass);
        if ("1" == cdcSubsystem->getIdUsbSerial()){
            PDM_LOG_DEBUG("CdcDevice:%s line: %d It is USB to serial device", __FUNCTION__, __LINE__);
            m_deviceType = USB2SERIAL;
            m_deviceSubType = cdcSubsystem->getUsbSerialSubType();
        }
        if((cdcSubsystem->getUsbModemId() == YES))
        {
            PDM_LOG_DEBUG("CdcDevice:%s line: %d It is modem dongle device", __FUNCTION__, __LINE__);
            m_deviceType = MODEM_DONGLE;
            m_devSpeed = USB_HIGH_SPEED;
        }
    }
}

void CdcDevice::updateDeviceInfo(DeviceClass* devClass)
{
    CdcSubSystem *cdcSubsystem = (CdcSubSystem *)devClass;
    if((devClass->getSubsystemName()== "net") && (m_deviceType != MODEM_DONGLE)){
        PDM_LOG_DEBUG("CdcDevice:%s line: %d It is Net device", __FUNCTION__, __LINE__);
        if(!(devClass->getUsbDriver().empty()))
            m_deviceSubType = devClass->getUsbDriver();

        if(!(cdcSubsystem->getNetIfIndex().empty()))
            m_ifindex = cdcSubsystem->getNetIfIndex();

        if(!(cdcSubsystem->getNetLinkMode().empty()))
            m_linkMode = cdcSubsystem->getNetLinkMode();

        if(!(cdcSubsystem->getNetDuplex().empty()))
            m_duplex = cdcSubsystem->getNetDuplex();

        if(!(cdcSubsystem->getNetAddress().empty()))
            m_address = cdcSubsystem->getNetAddress();

        if(!(cdcSubsystem->getNetOperState().empty()))
            m_operstate = cdcSubsystem->getNetOperState();

         m_isToastRequired = false;
    }
}