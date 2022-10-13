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

#ifndef _CDCDEVICE_H_
#define _CDCDEVICE_H_

#include "Common.h"
#include "Device.h"
#include "DeviceClass.h"

class CdcDevice : public Device
{
    std::string m_ifindex;
    std::string m_linkMode;
    std::string m_duplex;
    std::string m_address;
    std::string m_operstate;
public:
    CdcDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    ~CdcDevice(){}
    void setDeviceInfo(DeviceClass*);
	//void setDeviceInfo(PdmNetlinkEvent* pNE);
    void updateDeviceInfo(DeviceClass*);
	//void updateDeviceInfo(PdmNetlinkEvent* pNE);
    std::string& getDuplex(){return m_duplex;}
    std::string& getDeviceAddress(){return m_address;}
    std::string& getOperstate(){return m_operstate;}
    std::string& getDeviceifindex(){return m_ifindex;}
    std::string& getDeviceLinkMode(){return m_linkMode;}
};

#endif // CDCDEVICE_H
