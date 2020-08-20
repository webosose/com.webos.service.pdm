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

#include "DeviceManager.h"
#include "PluginAdapter.h"
#include "PdmLogUtils.h"
#include "LunaIPC.h"
#include <dlfcn.h>
#include <unistd.h>

PluginAdapter::PluginAdapter(): m_powerState(false)
    , m_plugin(nullptr)
    , m_handle(nullptr)
{
    PDM_LOG_INFO("PluginAdapter",0," Starting...");
}

PluginAdapter::~PluginAdapter() {
    PDM_LOG_INFO("PluginAdapter",0," Ending...");
    if(m_plugin) {
        m_plugin->deInit();
        delete m_plugin;
        dlclose(m_handle);
    }
}

void PluginAdapter::pluginLoader(LSHandle *lunaHandle,const std::string &path) {

    PDM_LOG_INFO("PluginAdapter",0," pluginLoading...");
    m_handle = dlopen(path.c_str(), RTLD_NOW);
    if (!m_handle) {
        PDM_LOG_ERROR("PluginAdapter:%s line: %d Fail to load plugin.so: [%s] ", __FUNCTION__, __LINE__,dlerror());
        return ;
    }
    auto instantiateFunc = reinterpret_cast<Plugin* (*)(PluginAdapter *, LSHandle *)> (dlsym(m_handle, "instantiatePlugin"));

    if(!instantiateFunc) {
        PDM_LOG_ERROR("PluginAdapter:%s line: %d Failed to find instantiatePlugin: [%s]", __FUNCTION__, __LINE__,dlerror());
        dlclose(m_handle);
        return ;
    }
    try {
        m_plugin = instantiateFunc(this, lunaHandle);
    } catch (...) {
        dlclose(m_handle);
    }
}

void PluginAdapter::init() {
    PDM_LOG_INFO("PluginAdapter",0," init...");
    if(m_plugin)
        m_plugin->init();
}

int PluginAdapter::getDeviceNumber(IDevice &device) const {
    if(m_plugin)
        return m_plugin->getDeviceNumber(device);
    return 0;
}

void PluginAdapter::notifyChange(const int &eventID, const int &eventType, IDevice* device) {
    if(m_plugin)
        m_plugin->notifyChange(eventID, eventType, device);
    return;
}

void PluginAdapter::update(const int &eventID, const int &eventType, IDevice* device) {
    PDM_LOG_INFO("PluginAdapter",0," update: [%d]",eventType);
    DeviceManager::getInstance()->HandlePluginEvent(eventType);
    if(eventType == POWER_STATE_RESUME_DONE) {
        resumeDone();
        PDM_LOG_DEBUG("PluginAdapter: %s line: %d Event Type %d", __FUNCTION__, __LINE__, eventType );
    }
}

void PluginAdapter::unloadPlugin() {
    if(m_plugin)
        m_plugin->deInit();
}

bool PluginAdapter::getPowerState() {
    return m_powerState;
}

void PluginAdapter::resumeDone() {
    m_powerState = true;
#ifdef WEBOS_SESSION
    LunaIPC::getInstance()->getResumeDone();
#endif
}

bool PluginAdapter::isUmoutBlocked(const std::string &path) {
    PDM_LOG_INFO("PluginAdapter",0," isUmoutBlocked: [%s]",path.c_str());
    if(m_plugin)
            return m_plugin->isUmoutBlocked(path);
    return false;
}
