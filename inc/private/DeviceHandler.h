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

#ifndef DEVICEHANDLER_H_
#define DEVICEHANDLER_H_

#include <list>
#include <string>
#include <algorithm>

#include "CommandManager.h"
#include "DeviceStateObserver.h"
#include "PdmConfig.h"
#include "PdmLunaHandler.h"
#include "PluginAdapter.h"
#include "CommandTypes.h"
#include "DeviceClass.h"
#include "PdmLogUtils.h"

// class PdmNetlinkEvent;
//class DeviceClass;

class DeviceHandler: public DeviceStateObserver {
protected:
    PdmConfig* const m_pConfObj;
    PluginAdapter* const m_pluginAdapter;
    std::string m_handlerName;
public:
    PdmLunaHandler *lunaHandler;
    DeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
                           : m_pConfObj(pConfObj),m_pluginAdapter(pluginAdapter),m_handlerName(""){
        lunaHandler = PdmLunaHandler::getInstance();
    }
    virtual ~DeviceHandler(){}
    // virtual bool HandlerEvent(PdmNetlinkEvent* event) = 0;
    virtual bool HandlerEvent(DeviceClass* deviceClass) = 0;
    virtual bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) = 0;
    virtual bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) = 0;
    virtual bool HandlePluginEvent(int eventType);
    void commandResponse(CommandResponse *cmdResponse, PdmDevStatus result);
    virtual std::string getHandlerName() { return m_handlerName; }
};
//To get the device with number
template < class T >  T* getDeviceWithNum (std::list<T*>& sList, int devNum){
    if(sList.empty())
        return nullptr;

    for (auto deviceList: sList){
        if(deviceList->getDeviceNum() == devNum)
            return deviceList;
    }
    return nullptr;
}
//To get the device with path
template < class T >  T* getDeviceWithPath(std::list<T*>& sList, std::string devPath){

    if(sList.empty())
        return nullptr;
PDM_LOG_DEBUG("DeviceHandler:%s line: %d",__FUNCTION__, __LINE__);
    for (auto deviceList: sList){
        // if(deviceList->getDevicePath().empty()) {
        //     deviceList->m_devicePath = devPath;
        // }
        PDM_LOG_DEBUG("DeviceHandler:%s line: %d Existing Path: %s NewPath: %s",__FUNCTION__, __LINE__, deviceList->getDevicePath().c_str(), devPath.c_str());
        std::string devicePath = deviceList->getDevicePath();
        PDM_LOG_DEBUG("DeviceHandler:%s line: %d",__FUNCTION__, __LINE__);
        if( devPath.compare(0,devicePath.length(),devicePath) == 0 )
            return deviceList;
        PDM_LOG_DEBUG("DeviceHandler:%s line: %d",__FUNCTION__, __LINE__);
    }
    PDM_LOG_DEBUG("DeviceHandler:%s line: %d",__FUNCTION__, __LINE__);
    return nullptr;
}
//To get the device with name
template < class T >  T* getDeviceWithName(std::list<T*>& sList, std::string devName)
{
    if(sList.empty())
        return nullptr;

    for (auto deviceList: sList){
        std::string deviceName = deviceList->getDeviceName();
        if(devName.compare(0,deviceName.length(),deviceName) == 0)
            return deviceList;
    }
    return nullptr;
}

#endif /* DEVICEHANDLER_H_ */
