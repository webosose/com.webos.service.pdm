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
    SoundSubsystem* soundSubsystem = (SoundSubsystem*)devClassPtr;

    PDM_LOG_DEBUG("SoundDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if(!(soundSubsystem->getSpeed()).empty()) {
        // m_devSpeed = getDeviceSpeed(stoi(soundSubsystem->getDevSpeed(),nullptr));
        m_devSpeed = getDeviceSpeed(stoi(soundSubsystem->getSpeed()));
    }
    PDM_LOG_DEBUG("SoundDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if(!soundSubsystem->getDevPath().empty()) {
        m_devicePath = soundSubsystem->getDevPath();
    }
PDM_LOG_DEBUG("SoundDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    Device::setDeviceInfo(soundSubsystem);
}

void SoundDevice::updateDeviceInfo(DeviceClass* devClassPtr)
{
    PDM_LOG_DEBUG("SoundDevice:%s line: %d", __FUNCTION__, __LINE__);
    SoundSubsystem* soundSubsystem = (SoundSubsystem*)devClassPtr;
    
    PDM_LOG_DEBUG("SoundDevice:%s line: %d", __FUNCTION__, __LINE__);
    if(soundSubsystem->getSubsystemName() == "sound") {
        if(!soundSubsystem->getUsbDriver().empty())
            m_deviceSubType = soundSubsystem->getUsbDriver();
        PDM_LOG_DEBUG("SoundDevice:%s line: %d", __FUNCTION__, __LINE__);
        if(soundSubsystem->getDevPath().find("usb") != std::string::npos)
            m_builtIn = false;

        if(!soundSubsystem->getCardName().empty())
            m_soundDeviceName = soundSubsystem->getCardName();

        if(!soundSubsystem->getCardNumber().empty())
            m_cardNumber = stoi(soundSubsystem->getCardNumber());

        if(!(soundSubsystem->getCardId().empty()))
            m_cardId = soundSubsystem->getCardId();

        SoundSubDevice* subDevice = getSubDevice(soundSubsystem->getDevName());
        PDM_LOG_DEBUG("SoundDevice:%s line: %d", __FUNCTION__, __LINE__);
        switch (sMapDeviceActions[soundSubsystem->getAction()]) {
            PDM_LOG_DEBUG("SoundDevice:%s line: %d", __FUNCTION__, __LINE__);
                case DeviceActions::USB_DEV_ADD:
                PDM_LOG_DEBUG("SoundDevice:%s line: %d", __FUNCTION__, __LINE__);
                    if (!soundSubsystem->getDevName().empty()) {
                    if (subDevice) {
                        PDM_LOG_DEBUG("SoundDevice:%s line: %d", __FUNCTION__, __LINE__);
                            subDevice->updateInfo(soundSubsystem->getDevName(), findDeviceType(soundSubsystem->getDevPath()));
                        }
                        else {
                            PDM_LOG_DEBUG("SoundDevice:%s line: %d", __FUNCTION__, __LINE__);
                            subDevice = new (std::nothrow) SoundSubDevice(soundSubsystem->getDevName(), findDeviceType(soundSubsystem->getDevPath()));
                            if (!subDevice) {
                                PDM_LOG_CRITICAL("SoundDevice:%s line: %d Not able to create the sub device", __FUNCTION__, __LINE__);
                                return;
                            }
                            mSubDeviceList.push_back(subDevice);
                            PDM_LOG_DEBUG("SoundDevice:%s line: %d", __FUNCTION__, __LINE__);
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
