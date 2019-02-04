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

#ifndef MTPDEVICE_H
#define MTPDEVICE_H

#include <functional>
#include "Storage.h"
#include "PdmErrors.h"

class MTPDevice : public Storage
{
private:
    using handlerCb = std::function<void(EventType,MTPDevice*)>;
    handlerCb mMtpDeviceHandlerCb;
    bool mountDevice(const std::string &mtpDeviceLink);
    bool unmountDevice() const;

public:
    MTPDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    ~MTPDevice();
    void setDeviceInfo(PdmNetlinkEvent* pNE);
    void setMountPath(std::string newMountName) {
        mountName = newMountName;
    }
    PdmDevStatus mtpMount( const std::string &mtpDeviceLink );
    PdmDevStatus mtpUmount();
    PdmDevStatus eject();
    void onDeviceRemove();
    void resumeRequest(const int &eventType);
    void registerCallback(handlerCb mtpDeviceHandlerCb);
};
#endif // MTPDEVICE_H
