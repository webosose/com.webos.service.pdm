// Copyright (c) 2019-2024 LG Electronics, Inc.
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

#include "VideoDevice.h"
#include "Common.h"
#include "PdmLogUtils.h"
#include "DeviceClass.h"
#include "VideoSubsystem.h"

using namespace PdmDevAttributes;

VideoDevice::VideoDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
                : Device(pConfObj, pluginAdapter, "CAM", PDM_ERR_NOTHING)
                , m_subSystem("")
                , m_kernel(""){
}

void VideoSubDevice::updateInfo(std::string devName, std::string capabilities, std::string productName, std::string version) {
    m_devPath = "/dev/" + devName;
    m_capabilities = capabilities;
    m_productName = std::move(productName);
    m_version = version;
}

VideoDevice::~VideoDevice() {
    for (auto subDevice : mSubDeviceList) {
        delete subDevice;
    }
    mSubDeviceList.clear();
}

void VideoDevice::setDeviceInfo(DeviceClass* devClassPtr, bool isCameraReady)
{
    if(devClassPtr->getAction() == DEVICE_ADD ) {
        PDM_LOG_DEBUG("VideoDevice:%s line: %d setDeviceInfo: DEVICE_ADD", __FUNCTION__, __LINE__);
        if(!devClassPtr->getSpeed().empty()) {
            m_devSpeed = getDeviceSpeed(stoi(devClassPtr->getSpeed(), nullptr));
        }
        if(!isCameraReady) {
            m_deviceType = DEV_TYPE_UNKNOWN;
        }
        Device::setDeviceInfo(devClassPtr);
    }
}

void VideoDevice::updateDeviceInfo(DeviceClass* devClassPtr)
{
	VideoSubsystem* videoSubSystem = (VideoSubsystem*)devClassPtr;

#ifdef WEBOS_SESSION
    if (!devClassPtr->getDevName().empty()) {
        std::string devPath = "/dev/";
        m_devPath = devPath.append(devClassPtr->getDevName());
    }
#endif
    if (devClassPtr->getSubsystemName() ==  "video4linux" && videoSubSystem->getCapabilities().find(":capture:") !=  std::string::npos) {
        if(!devClassPtr->getSubsystemName().empty())
            m_subSystem = devClassPtr->getSubsystemName();

        if(!devClassPtr->getUsbDriver().empty())
            m_deviceSubType = devClassPtr->getUsbDriver();

        if (!devClassPtr->getDevName().empty()) {
            std::string cam_path = "/dev/";
            m_kernel = cam_path.append(devClassPtr->getDevName());
        }

        VideoSubDevice* subDevice = getSubDevice("/dev/" + devClassPtr->getDevName());
        switch (sMapDeviceActions[devClassPtr->getAction()]) {
            case DeviceActions::USB_DEV_ADD:
                if (!devClassPtr->getDevName().empty()) {
                    if (subDevice) {
                        subDevice->updateInfo(devClassPtr->getDevName(), videoSubSystem->getCapabilities(), videoSubSystem->getProductName(), videoSubSystem->getVersion());
                    }
                    else {
                        subDevice = new (std::nothrow) VideoSubDevice(devClassPtr->getDevName(), videoSubSystem->getCapabilities(), videoSubSystem->getProductName(), videoSubSystem->getVersion());
                        if (!subDevice) {
                            PDM_LOG_CRITICAL("VideoDevice:%s line: %d Not able to create the sub device", __FUNCTION__, __LINE__);
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

VideoSubDevice* VideoDevice::getSubDevice(std::string devPath) {
    for (auto videoSubDevice : mSubDeviceList) {
        if (videoSubDevice->getDevPath() == devPath) {
            return videoSubDevice;
        }
    }
    return nullptr;
}
