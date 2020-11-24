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

#include "SoundDevice.h"
#include "Common.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

SoundDevice::SoundDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
                         :Device(pConfObj, pluginAdapter,  "SOUND", PDM_ERR_NOTHING)
                          , m_cardName("card")
                          , m_cardNumber(0)
                          , m_soundDeviceName("")
                          , m_cardId("")
{

}

void SoundDevice::setDeviceInfo(PdmNetlinkEvent* pNE)
{
    PDM_LOG_DEBUG("SoundDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if( pNE->getDevAttribute(DEVTYPE) == USB_DEVICE ) {
        if(!pNE->getDevAttribute(SPEED).empty()) {
            m_devSpeed = getDeviceSpeed(stoi(pNE->getDevAttribute(SPEED),nullptr));
        }
        Device::setDeviceInfo(pNE);
    }
}

void SoundDevice::updateDeviceInfo(PdmNetlinkEvent* pNE)
{
    if(pNE->getDevAttribute(SUBSYSTEM) == "sound"){
        if(!(pNE->getDevAttribute(CARD_ID).empty()))
            m_cardId = pNE->getDevAttribute(CARD_ID);
        if(!(pNE->getDevAttribute(CARD_NUMBER).empty())){
            m_cardNumber = stoi(pNE->getDevAttribute(CARD_NUMBER));
            m_soundDeviceName = m_cardName.append(std::to_string(m_cardNumber));
        }
    }
}
