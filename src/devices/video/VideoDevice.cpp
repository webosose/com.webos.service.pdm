// Copyright (c) 2019-2021 LG Electronics, Inc.
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
    m_productName = productName;
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
    PDM_LOG_DEBUG("VideoDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
	if (devClassPtr->getDevType() != "Video")	return;

	VideoSubsystem* videoSubSystem = (VideoSubsystem*)devClassPtr;
	if (videoSubSystem == nullptr) return;

    if(videoSubSystem->getAction() == DEVICE_ADD ) {
        PDM_LOG_DEBUG("VideoDevice:%s line: %d setDeviceInfo: DEVICE_ADD", __FUNCTION__, __LINE__);
        if(!videoSubSystem->getDevSpeed().empty()) {
            m_devSpeed = getDeviceSpeed(stoi(videoSubSystem->getDevSpeed(), nullptr));
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
	if (videoSubSystem == nullptr) return;

#ifdef WEBOS_SESSION
    if (!videoSubSystem->getDevName().empty()) {
        std::string devPath = "/dev/";
        m_devPath = devPath.append(videoSubSystem->getDevName());
    }
#endif
    if (videoSubSystem->getSubsystemName() ==  "video4linux" && videoSubSystem->getCapabilities() ==  ":capture:") {
        if(!videoSubSystem->getSubsystemName().empty())
            m_subSystem = videoSubSystem->getSubsystemName();

        if(!videoSubSystem->getUsbDriverId().empty())
            m_deviceSubType = videoSubSystem->getUsbDriverId();

        if (!videoSubSystem->getDevName().empty()) {
            std::string cam_path = "/dev/";
            m_kernel = cam_path.append(videoSubSystem->getDevName());
        }

        VideoSubDevice* subDevice = getSubDevice("/dev/" + videoSubSystem->getDevName());
        switch (sMapDeviceActions[videoSubSystem->getAction()]) {
            case DeviceActions::USB_DEV_ADD:
                if (!videoSubSystem->getDevName().empty()) {
                    if (subDevice) {
                        subDevice->updateInfo(videoSubSystem->getDevName(), videoSubSystem->getCapabilities(), videoSubSystem->getProductName(), videoSubSystem->getVersion());
                    }
                    else {
                        subDevice = new (std::nothrow) VideoSubDevice(videoSubSystem->getDevName(), videoSubSystem->getCapabilities(), videoSubSystem->getProductName(), videoSubSystem->getVersion());
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

#if 0
void VideoDevice::setDeviceInfo(PdmNetlinkEvent* pNE, bool isCameraReady)
{
    PDM_LOG_DEBUG("VideoDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if(pNE->getDevAttribute(ACTION) == DEVICE_ADD ) {
        PDM_LOG_DEBUG("VideoDevice:%s line: %d setDeviceInfo: DEVICE_ADD", __FUNCTION__, __LINE__);
        if(!pNE->getDevAttribute(SPEED).empty()) {
            m_devSpeed = getDeviceSpeed(stoi(pNE->getDevAttribute(SPEED),nullptr));
        }
        if(!isCameraReady) {
            m_deviceType = DEV_TYPE_UNKNOWN;
        }
        Device::setDeviceInfo(pNE);
    }
}

void VideoDevice::updateDeviceInfo(PdmNetlinkEvent* pNE)
{
#ifdef WEBOS_SESSION
    if (!pNE->getDevAttribute(DEVNAME).empty()) {
        std::string devPath = "/dev/";
        m_devPath = devPath.append(pNE->getDevAttribute(DEVNAME));
    }
#endif
    if (pNE->getDevAttribute(SUBSYSTEM) ==  "video4linux" && pNE->getDevAttribute(ID_V4L_CAPABILITIES) ==  ":capture:") {
        if(!pNE->getDevAttribute(SUBSYSTEM).empty())
            m_subSystem = pNE->getDevAttribute(SUBSYSTEM);

        if(!pNE->getDevAttribute(ID_USB_DRIVER).empty())
            m_deviceSubType = pNE->getDevAttribute(ID_USB_DRIVER);

        if (!pNE->getDevAttribute(DEVNAME).empty()) {
            std::string cam_path = "/dev/";
            m_kernel = cam_path.append(pNE->getDevAttribute(DEVNAME));
        }

        VideoSubDevice* subDevice = getSubDevice("/dev/"+pNE->getDevAttribute(DEVNAME));
        switch (sMapDeviceActions[pNE->getDevAttribute(ACTION)]) {
            case DeviceActions::USB_DEV_ADD:
                if (!pNE->getDevAttribute(DEVNAME).empty()) {
                    if (subDevice) {
                        subDevice->updateInfo(pNE->getDevAttribute(DEVNAME), pNE->getDevAttribute(ID_V4L_CAPABILITIES), pNE->getDevAttribute(ID_V4L_PRODUCT), pNE->getDevAttribute(ID_V4L_VERSION));
                    }
                    else {
                        subDevice = new (std::nothrow) VideoSubDevice(pNE->getDevAttribute(DEVNAME), pNE->getDevAttribute(ID_V4L_CAPABILITIES), pNE->getDevAttribute(ID_V4L_PRODUCT), pNE->getDevAttribute(ID_V4L_VERSION));
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
#endif

VideoSubDevice* VideoDevice::getSubDevice(std::string devPath) {
    for (auto videoSubDevice : mSubDeviceList) {
        if (videoSubDevice->getDevPath() == devPath) {
            return videoSubDevice;
        }
    }
    return nullptr;
}
