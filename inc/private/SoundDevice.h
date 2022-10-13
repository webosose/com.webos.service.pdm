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

#ifndef _SOUNDDEVICE_H_
#define _SOUNDDEVICE_H_

#include "Common.h"
#include "Device.h"
#include <functional>

class SoundDevice : public Device
{
    std::string m_cardName;
    int m_cardNumber;
    std::string m_soundDeviceName;
    std::string m_cardId;
public:
    SoundDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    ~SoundDevice() = default;
    // void setDeviceInfo(PdmNetlinkEvent* pNE);
    // void updateDeviceInfo(PdmNetlinkEvent* pNE);
    void setDeviceInfo(DeviceClass* pNE);
    void updateDeviceInfo(DeviceClass* pNE);
    std::string getCardName(){ return m_soundDeviceName;}
    std::string getCardId(){ return m_cardId;}
    int getCardNumber(){ return m_cardNumber;}
};

#endif // SOUNDDEVICE_H
