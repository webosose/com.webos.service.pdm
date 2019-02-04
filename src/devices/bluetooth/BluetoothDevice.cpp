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

#include "BluetoothDevice.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

void BluetoothDevice::setDeviceInfo(PdmNetlinkEvent* pNE)
{
    PDM_LOG_DEBUG("BluetoothDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if(pNE->getDevAttribute(ACTION) == DEVICE_ADD ){
        PDM_LOG_DEBUG("BluetoothDevice:%s line: %d setDeviceInfo: DEVICE_ADD", __FUNCTION__, __LINE__);
        m_devSpeed = getDeviceSpeed(stoi(pNE->getDevAttribute(SPEED),nullptr));
        Device::setDeviceInfo(pNE);
        /*m_deviceSubType will be modified once  requirement will be clear*/
        m_deviceSubType = pNE->getDevAttribute(ID_MODEL);
    }
}
