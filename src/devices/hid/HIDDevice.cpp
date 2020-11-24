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

#include "HIDDevice.h"
#include "Common.h"

using namespace PdmDevAttributes;

void HIDDevice::setDeviceInfo(PdmNetlinkEvent* pNE)
{
    if(pNE->getDevAttribute(ACTION) == DEVICE_ADD ) {
        Device::setDeviceInfo(pNE);
        if( pNE->getDevAttribute(DEVTYPE) == USB_DEVICE ){
            if(!pNE->getDevAttribute(SPEED).empty()) {
                m_devSpeed = getDeviceSpeed(stoi(pNE->getDevAttribute(SPEED),nullptr));
            }
        }else if(pNE->getDevAttribute(PROCESSED) == YES ) {
            mHidDeviceHandlerCb(ADD,nullptr);
        }
    }
}

void HIDDevice::registerCallback(handlerCb hidDeviceHandlerCb) {
    mHidDeviceHandlerCb = hidDeviceHandlerCb;
}

