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

#include "SoundDevice.h"
#include "Common.h"
#include "PdmLogUtils.h"
#include "SoundSubsystem.h"

using namespace PdmDevAttributes;

SoundDevice::SoundDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
                         :Device(pConfObj, pluginAdapter,  "SOUND", PDM_ERR_NOTHING)
                          , m_cardName("")
                          , m_cardNumber(0)
                          , m_soundDeviceName("")
                          , m_cardId("")
                          , m_builtIn(true)
{

}

void SoundSubDevice::updateInfo(std::string devName, std::string deviceType) {
    m_devPath = "/dev/" + devName;
    m_devType = deviceType;
}

void SoundDevice::setDeviceInfo(DeviceClass* devClassPtr)
{
    PDM_LOG_DEBUG("SoundDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    SoundSubsystem* soundSubSystem = (SoundSubsystem*)devClassPtr;
	if (soundSubSystem == nullptr) return;

    PDM_LOG_DEBUG("SoundDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if(!(soundSubSystem->getSpeed()).empty()) {
        // m_devSpeed = getDeviceSpeed(stoi(soundSubSystem->getDevSpeed(),nullptr));
        m_devSpeed = getDeviceSpeed(stoi(soundSubSystem->getSpeed()));
    }
    PDM_LOG_DEBUG("SoundDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if(!soundSubSystem->getDevPath().empty()) {
        m_devicePath = soundSubSystem->getDevPath();
    }
PDM_LOG_DEBUG("SoundDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    Device::setDeviceInfo(soundSubSystem);
}

void SoundDevice::updateDeviceInfo(DeviceClass* devClassPtr)
{
    SoundSubsystem* soundSubSystem = (SoundSubsystem*)devClassPtr;
	if (soundSubSystem == nullptr) return;

    if(soundSubSystem->getSubsystemName() == "sound") {
        if(!soundSubSystem->getUsbDriver().empty())
            m_deviceSubType = soundSubSystem->getUsbDriver();

        if(soundSubSystem->getDevPath().find("usb") != std::string::npos)
            m_builtIn = false;

        if(!soundSubSystem->getCardName().empty())
            m_soundDeviceName = soundSubSystem->getCardName();

        if(!soundSubSystem->getCardNumber().empty())
            m_cardNumber = stoi(soundSubSystem->getCardNumber());

        if(!(soundSubSystem->getCardId().empty()))
            m_cardId = soundSubSystem->getCardId();

        SoundSubDevice* subDevice = getSubDevice(soundSubSystem->getDevName());
        switch (sMapDeviceActions[soundSubSystem->getAction()]) {
                case DeviceActions::USB_DEV_ADD:
                    if (!soundSubSystem->getDevName().empty()) {
                    if (subDevice) {
                            subDevice->updateInfo(soundSubSystem->getDevName(), findDeviceType(soundSubSystem->getDevPath()));
                        }
                        else {
                            subDevice = new (std::nothrow) SoundSubDevice(soundSubSystem->getDevName(), findDeviceType(soundSubSystem->getDevPath()));
                            if (!subDevice) {
                                PDM_LOG_CRITICAL("SoundDevice:%s line: %d Not able to create the sub device", __FUNCTION__, __LINE__);
                                return;
                            }
                            mSubDeviceList.push_back(subDevice);
                        }
                    }
                    break;
            case DeviceActions::USB_DEV_REMOVE:
                    if (subDevice) {
                        mSubDeviceList.remove(subDevice);
                        delete subDevice;
                    }
                    break;
                default:
                    //Do nothing
                    break;
        }
    }
}

std::string SoundDevice::findDeviceType(std::string devicePath)
{
    std::string devType = "misc";
    if (devicePath.find("pcm") != std::string::npos) {
    if (devicePath[devicePath.length() - 1] == 'c')
            devType = "capture";
    else if (devicePath[devicePath.length() - 1] == 'p')
            devType = "playback";
    } else if (devicePath.find("control") != std::string::npos)
        devType = "control";

    return devType;
}

SoundSubDevice* SoundDevice::getSubDevice(std::string devName) {
    for (auto soundSubDevice : mSubDeviceList) {
        if (soundSubDevice->getDevPath() == ("/dev/" + devName))
            return soundSubDevice;
    }
    return nullptr;
}
