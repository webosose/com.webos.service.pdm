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

#ifndef PTPDEVICE_H
#define PTPDEVICE_H

#include <functional>
#include "Storage.h"
#include "PdmErrors.h"

class PTPDevice : public Storage
{
private:
    int32_t m_ptpDevNum;
    std::string m_driveStatus;
    using handlerCb = std::function<void(EventType,PTPDevice*)>;
    handlerCb mPtpDeviceHandlerCb;
    PdmDevStatus ptpUmount();
    PdmDevStatus ptpMount();
    bool checkDirectory(const std::string &path);
    bool checkEmpty(const std::string &path);
public:
    PTPDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    ~PTPDevice() = default;
    void setDeviceInfo(PdmNetlinkEvent* pNE);
    void onDeviceRemove();
    void resumeRequest(const int &eventType);
    PdmDevStatus eject();
    bool mountDevice();
    bool unmountDevice() const;
    void registerCallback(handlerCb ptpDeviceHandlerCb);
   const std::string getDriveStatus(){return m_driveStatus;}
};

#endif // PTPDEVICE_H
