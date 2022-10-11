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

#ifndef _SOUNDDEVICEHANDLER_H_
#define _SOUNDDEVICEHANDLER_H_

#include "DeviceHandler.h"
#include "SoundDevice.h"
#include "PdmDeviceFactory.h"
#include "PdmNetlinkEvent.h"
#include "PdmLogUtils.h"
#include "Common.h"


class SoundDeviceHandler : public DeviceHandler
{
private:
    const std::string iClass = ":01";
    std::list<SoundDevice*> sList;
    bool m_deviceRemoved;

    SoundDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    static bool mIsObjRegistered;

    //Register Object to object factory. This is called automatically
    static bool RegisterObject() {
        return (PdmDeviceFactory::getInstance()->Register("SOUND",
                                                          &SoundDeviceHandler::CreateObject));
    }

    void removeDevice(SoundDevice* hdl);
    bool isSoundDevice(PdmNetlinkEvent* pNE);

public:
    virtual ~SoundDeviceHandler();

    static DeviceHandler* CreateObject(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) {
        if (mIsObjRegistered) {
            PDM_LOG_DEBUG("CreateObject - Creating the sound device handler");
            return new SoundDeviceHandler(pConfObj, pluginAdapter);
        } else {
            return nullptr;
        }
    }

    bool HandlerEvent(PdmNetlinkEvent* pNE) override;
    bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) override;
    bool HandlePluginEvent(int eventType);
    bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) override;
    bool GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message);
    bool GetAttachedAudioDeviceList(pbnjson::JValue &payload, LSMessage *message);
    bool GetAttachedAudioSubDeviceList(pbnjson::JValue &payload, LSMessage *message);
    void ProcessSoundDevice(PdmNetlinkEvent* pNE);
};

#endif // SOUNDDEVICEHANDLER_H
