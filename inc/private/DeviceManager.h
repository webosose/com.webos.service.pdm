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

#ifndef _DEVICEMANAGER_H
#define _DEVICEMANAGER_H


#include <cstdarg>
#include <list>
#include <string>

#include "CommandManager.h"
#include "CommandTypes.h"
#include "PdmLocaleHandler.h"
#include "PluginAdapter.h"

class DeviceHandler;
class PdmNetlinkEvent;
class PdmConfig;

class DeviceManager {

private:
    int m_UsbDeviceCount;
    int m_maxUsbDevices;
    PdmConfig* m_pConfObj;
    PluginAdapter* m_pluginAdapter;
    PdmLocaleHandler *m_pLocHandler;
    std::list<DeviceHandler*> mHandlerList;

public:

    DeviceManager();
    ~DeviceManager();
    bool init(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    bool HandlePdmDevice(PdmNetlinkEvent *evn);
    bool HandlePdmCommand(CommandType *cmdtypes, CommandResponse *cmdResponse);
    bool HandlePluginEvent(int eventType);
    void updateDeviceCount(int eventID);
    std::list<DeviceHandler*> getDeviceHandlerList();
    static DeviceManager *getInstance();
    int getDeviceCount() { return m_UsbDeviceCount; }
    int getStorageDevCount();

private:

    void createAlertForMaxUsbDevices(const std::string &alertId);
    bool maxUsbDevicesExceeded();
    bool createPdmdeviceList();
    int readMaxUsbDevices();

};

#endif //_DEVICEMANAGER_H
