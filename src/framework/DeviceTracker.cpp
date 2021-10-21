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

#include "Common.h"
#include "DeviceTracker.h"
#include "DeviceHandler.h"
#include "DeviceManager.h"
#include "PdmLogUtils.h"
#include "LunaIPC.h"

using namespace PdmDevAttributes;

DeviceTracker::DeviceTracker(PluginAdapter* const pluginAdapter)
     :IObserver(), m_pluginAdapter(pluginAdapter)
{
}

DeviceTracker::~DeviceTracker()
{
    std::list<DeviceHandler*> pDeviceHandlerList = DeviceManager::getInstance()->getDeviceHandlerList();

    for(auto handler : pDeviceHandlerList)
        handler->Unregister(this);
}

void DeviceTracker::RegisterCallback(lunaCb func)
{
    mLunaServiceCallback = func;
}

void DeviceTracker::attachObservers()
{
    std::list<DeviceHandler*> pDeviceHandlerList = DeviceManager::getInstance()->getDeviceHandlerList();

    for(auto handler : pDeviceHandlerList)
        handler->Register(this);
}

void DeviceTracker::update(const int &eventDeviceType, const int &eventID, IDevice* device)
{
    int eventType = UNKNOWN_DEVICE;
    std::string hubPortPath;
    PDM_LOG_DEBUG("DeviceTracker::update -  Event: %d eventID: %d", eventDeviceType, eventID);

    m_pluginAdapter->notifyChange(PdmEvent, eventID, device);
#ifdef WEBOS_SESSION
    if(device) {
        if (!device->getHubPortNumber().empty()) {
            hubPortPath = device->getHubPortNumber();
        }
    }
#endif

    if(eventDeviceType == VIDEO_DEVICE) {
        LunaIPC::getInstance()->notifyDeviceChange(VIDEO_DEVICE, eventID);
    }

    switch(eventID)
    {
        case ADD:
        case REMOVE:
        case MOUNT:
        case UMOUNT:
        case MOUNTALL:
        case UNMOUNTALL:
        case FORMAT_STARTED:
        case FORMAT_SUCCESS:
            if((eventDeviceType == STORAGE_DEVICE)||(eventDeviceType == MTP_DEVICE)
               || (eventDeviceType == PTP_DEVICE) )
                eventType = STORAGE_DEVICE;
            else if(eventDeviceType == ALL_DEVICE)
                eventType = ALL_DEVICE;
            else
                eventType = NON_STORAGE_DEVICE;
    }
    if(eventType != UNKNOWN_DEVICE)
        LunaIPC::getInstance()->notifyDeviceChange(eventType, eventID, hubPortPath);

}
