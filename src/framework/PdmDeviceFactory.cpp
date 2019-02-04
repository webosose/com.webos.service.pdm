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

#include "PdmConfig.h"
#include "PdmDeviceFactory.h"
#include <utility>

PdmDeviceFactory *PdmDeviceFactory::getInstance()
{
    static PdmDeviceFactory _instance;
    return &_instance;
}

bool PdmDeviceFactory::Register(const std::string &handlerName, pFuncHandlerCreator create)
{
    if (m_handlerCreatorsMap.find(handlerName)!=m_handlerCreatorsMap.end())
        return false;
    m_handlerCreatorsMap.insert(std::pair<const std::string,pFuncHandlerCreator>(handlerName,create));
    return true;
}

bool PdmDeviceFactory::UnRegister(const std::string &handlerName)
{
    DeviceHandler* handle = nullptr;
    std::map<std::string, DeviceHandler*>::iterator it = m_handlesMap.begin();

    for( ; it != m_handlesMap.end(); ++it){
        if(handlerName == it->first)
        {
            handle = it->second;
            delete handle;
            m_handlesMap.erase(handlerName);
            return true;
        }
    }

    return false;
}

DeviceHandler* PdmDeviceFactory::CreateObject(const std::string &handlerName, PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
{
     DeviceHandler* handle = nullptr;
     std::map<std::string, pFuncHandlerCreator>::iterator it = m_handlerCreatorsMap.begin();

     for( ; it != m_handlerCreatorsMap.end(); ++it){
         if(handlerName == it->first)
         {
             handle = it->second(pConfObj, pluginAdapter);
             m_handlesMap.insert(std::pair<const std::string,DeviceHandler*>(handlerName,handle));
         }
     }
     return handle;

}

