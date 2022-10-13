// Copyright (c) 2019-2022 LG Electronics, Inc.
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
#include <list>
#include "DeviceClass.h"

class SoundSubDevice {
private:
    std::string m_devPath;
    std::string m_devType;

public:
    SoundSubDevice(std::string devName, std::string deviceType)
        : m_devPath("/dev/" + devName),
        m_devType(deviceType) {
    };
    ~SoundSubDevice() = default;

    void updateInfo(std::string devPath, std::string deviceType);
    std::string getDevPath() {return m_devPath;}
    std::string getDevType() {return m_devType;}
};

class SoundDevice : public Device
{
    bool m_builtIn;
    std::string m_cardName;
    int m_cardNumber;
    std::string m_soundDeviceName;
    std::string m_cardId;
    std::list<SoundSubDevice*> mSubDeviceList;
    std::string findDeviceType(std::string devicePath);
public:
    SoundDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    ~SoundDevice() = default;
    void setDeviceInfo(DeviceClass* devClass);
    void updateDeviceInfo(DeviceClass* devClass);
    std::string getCardName(){ return m_soundDeviceName;}
    std::string getCardId(){ return m_cardId;}
    int getCardNumber(){ return m_cardNumber;}
    bool getBuiltIn(){return m_builtIn;}

    std::list<SoundSubDevice*> getSubDeviceList() const {return mSubDeviceList;}
    SoundSubDevice* getSubDevice(std::string devPath);
};

#endif // SOUNDDEVICE_H
