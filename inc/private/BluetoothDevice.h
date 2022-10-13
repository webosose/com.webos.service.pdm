// Copyright (c) 2019-2020 LG Electronics, Inc.
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

#ifndef _BLUETOOTHDEVICE_H_
#define _BLUETOOTHDEVICE_H_

#include "Device.h"
#include "Common.h"
#include "DeviceClass.h"

class BluetoothDevice : public Device
{
public:
    BluetoothDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
                    : Device(pConfObj, pluginAdapter, "BLUETOOTH", PdmDevAttributes::PDM_ERR_NOTHING){}
    ~BluetoothDevice() = default;
    void setDeviceInfo(DeviceClass*);
	//void setDeviceInfo(PdmNetlinkEvent* pNE);
#ifdef WEBOS_SESSION
    void updateDeviceInfo(DeviceClass*);
	//void updateDeviceInfo(PdmNetlinkEvent* pNE);

#endif
};

#endif // _BLUETOOTHDEVICE_H_

