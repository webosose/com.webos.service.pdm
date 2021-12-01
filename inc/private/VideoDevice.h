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

#ifndef _VIDEODEVICE_H_
#define _VIDEODEVICE_H_

#include "Device.h"
#include <functional>
#include <list>

const std::string ID_V4L_CAPABILITIES  =  "ID_V4L_CAPABILITIES";
const std::string ID_V4L_PRODUCT  =  "ID_V4L_PRODUCT";
const std::string ID_V4L_VERSION  =  "ID_V4L_VERSION";

class VideoDeviceHandler;
class VideoSubDevice {
private:
    std::string m_capabilities;
    std::string m_devPath;
    std::string m_productName;
    std::string m_version;

public:
    VideoSubDevice(std::string devName, std::string capabilities, std::string productName, std::string version)
        : m_devPath("/dev/"+devName),
        m_capabilities(capabilities),
        m_productName(productName),
        m_version(version) {
    };
    ~VideoSubDevice() = default;

    void updateInfo(std::string devName, std::string capabilities, std::string productName, std::string version);
    std::string getDevPath() {return m_devPath;}
    std::string getCapabilities() {return m_capabilities;}
    std::string getProductName() {return m_productName;}
    std::string getVersion() {return m_version;}
};

class VideoDevice : public Device
{
    std::string m_subSystem;
    std::string m_kernel;
    std::list<VideoSubDevice*> mSubDeviceList;
public:
    VideoDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    ~VideoDevice();
    void setDeviceInfo(PdmNetlinkEvent* pNE, bool isCameraReady);
    void updateDeviceInfo(PdmNetlinkEvent* pNE);
    std::string getSubsystem() {return m_subSystem;}
    std::string getKernel(){return m_kernel;}
    std::list<VideoSubDevice*> getSubDeviceList() const {return mSubDeviceList;}
    VideoSubDevice* getSubDevice(std::string devPath);
};

#endif // VIDEODEVICE_H
