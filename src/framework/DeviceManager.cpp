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

#include "BlackListDeviceHandler.h"
#include "Common.h"
#include "Device.h"
#include "DeviceHandler.h"
#include "PdmDeviceFactory.h"
#include "PdmLogUtils.h"
#include "PdmUtils.h"
#include "DeviceManager.h"
#include "StorageDeviceHandler.h"

using namespace PdmDevAttributes;

DeviceManager::DeviceManager() : m_pConfObj(nullptr),
                 m_pluginAdapter(nullptr) {

}

DeviceManager::~DeviceManager() {
}

DeviceManager *DeviceManager::getInstance() {
    static DeviceManager _instance;
    return &_instance;
}

bool DeviceManager::init(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
{
    m_pConfObj = pConfObj;
    m_pluginAdapter = pluginAdapter;
    return createPdmdeviceList();
}


bool DeviceManager::createPdmdeviceList() {

    HandlerNameToCreatorMap handlers = PdmDeviceFactory::getInstance()->getRegisteredHandlers();

    pbnjson::JValue supportedDevArray = pbnjson::JValue();
    PdmConfigStatus confErrCode = m_pConfObj->getValue("Common","SupportedDevices",supportedDevArray);
    if(confErrCode != PdmConfigStatus::PDM_CONFIG_ERROR_NONE)
    {
        PdmErrors::logPdmErrorCodeAndText(confErrCode);
        return false;
    }

    std::list<std::string> supportedDevList;
    for (const pbnjson::JValue supportedDev: supportedDevArray.items()) {
        if (supportedDev.isString())
            supportedDevList.push_back(supportedDev.asString());
    }

    for(HandlerNameToCreatorMap::iterator it = handlers.begin(); it != handlers.end(); ++it) {
        if (find (supportedDevList.begin(), supportedDevList.end(), it->first) != supportedDevList.end())
            mHandlerList.push_back(PdmDeviceFactory::getInstance()->CreateObject(it->first,m_pConfObj,m_pluginAdapter));
    }
    return true;
}

// bool DeviceManager::HandlePdmDevice(PdmNetlinkEvent *event) {

//     PDM_LOG_DEBUG("DeviceManager::HandlePdmDevice");

//     if (!(event->getDevAttribute(ID_BLACKLIST)).empty() && std::stoi(event->getDevAttribute(ID_BLACKLIST),nullptr)) {
//         PDM_LOG_DEBUG("Blacklist device detcted");
//         BlackListDeviceHandler blackListDeviceHandler(event);
//         return true;
//     }

//     for(auto handler : mHandlerList)
//     {
//         bool result = handler->HandlerEvent(event);
//         if(event->getDevAttribute(DEVTYPE) ==  "usb_device")
//             continue;
//         if(result)
//             return true;
//     }

//     return false;
// }

bool DeviceManager::HandlePdmDevice(DeviceClass *devClassPtr)
{
    // if (!(event->getDevAttribute(ID_BLACKLIST)).empty() && std::stoi(event->getDevAttribute(ID_BLACKLIST),nullptr)) {
    //     PDM_LOG_DEBUG("Blacklist device detcted");
    //     BlackListDeviceHandler blackListDeviceHandler(event);
    //     return true;
    // }

    if (!(devClassPtr->getIdBlackList()).empty() && std::stoi(devClassPtr->getIdBlackList(),nullptr)) {
        PDM_LOG_DEBUG("Blacklist device detected");
        BlackListDeviceHandler blackListDeviceHandler(devClassPtr);
        return true;
    }
    
    PDM_LOG_DEBUG("DeviceManager:%s line: %d", __FUNCTION__, __LINE__);
    for(auto handler : mHandlerList)
    {
        bool result = handler->HandlerEvent(devClassPtr);
        // if(event->getDevAttribute(DEVTYPE) ==  "usb_device")
        PDM_LOG_DEBUG("DeviceManager:%s line: %d", __FUNCTION__, __LINE__);
        PDM_LOG_DEBUG("DeviceManager:%s line: %d handlerName: %s, result: %d", __FUNCTION__, __LINE__, handler->getHandlerName().c_str(), result);
        if(devClassPtr->getDevType() ==  "usb_device")
            continue;
        if(result)
            return true;
    }
	return false;
}

bool DeviceManager::HandlePdmCommand(CommandType *cmdtypes, CommandResponse *cmdResponse){
    for(auto handler : mHandlerList)
    {
        if(handler->HandlerCommand(cmdtypes, cmdResponse))
            return true;
    }

    return false;
}

std::list<DeviceHandler*> DeviceManager::getDeviceHandlerList() {
    return mHandlerList;
}

bool DeviceManager::HandlePluginEvent(int eventType) {
    bool result = true;
    for(auto handler : mHandlerList) {
        if(handler->HandlePluginEvent(eventType) ==  false)
            result = false;
    }
    return result;
}
