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

#ifndef _BLUETOOTHDEVICEHANDLER_H_
#define _BLUETOOTHDEVICEHANDLER_H_

#include "CommandManager.h"
#include "DeviceHandler.h"
#include "BluetoothDevice.h"
#include "PdmDeviceFactory.h"
#include "PdmLogUtils.h"

class BluetoothDeviceHandler : public DeviceHandler
{
private:
    std::list<BluetoothDevice*> sList;
    static bool mIsObjRegistered;
    BluetoothDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    //Register Object to object factory. This is called automatically
    static bool RegisterObject() {
        return (PdmDeviceFactory::getInstance()->Register("BLUETOOTH",
                                                          &BluetoothDeviceHandler::CreateObject));
    }
    void removeDevice(BluetoothDevice* hdl);

public:
    ~BluetoothDeviceHandler();
    static DeviceHandler* CreateObject(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter){
        if (mIsObjRegistered){
            PDM_LOG_DEBUG("CreateObject - Creating the Bluetooth device handler");
            return new(std::nothrow) BluetoothDeviceHandler(pConfObj, pluginAdapter);
        }
        return nullptr;
    }
    bool HandlerEvent(PdmNetlinkEvent* pNE) override;
    bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) override;
    bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) override;
    bool GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message);
    void ProcessBluetoothDevice(PdmNetlinkEvent* pNE);
};

#endif // _BLUETOOTHDEVICEHANDLER_H_

