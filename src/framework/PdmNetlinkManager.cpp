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

#include "PdmNetlinkManager.h"
#include "DeviceTracker.h"
#include "PdmLogUtils.h"

PdmNetlinkManager *PdmNetlinkManager::getInstance()
{
    static PdmNetlinkManager _instance;
    return &_instance;
}

PdmNetlinkManager::PdmNetlinkManager():m_handler(nullptr)
{
}

PdmNetlinkManager::~PdmNetlinkManager()
{
    if(m_handler)
        delete m_handler;
    m_handler = nullptr;
}

int PdmNetlinkManager::start(CommandManager *cmdManager)
{
    m_handler = new (std::nothrow) PdmNetlinkHandler(cmdManager);

    if( m_handler && m_handler->start() ) {
        PDM_LOG_DEBUG("PdmNetlinkManager:%s line: %d NetlinkHandler started...", __FUNCTION__, __LINE__);
        return 0;
    }
    PDM_LOG_ERROR("PdmNetlinkManager:%s line: %d Unable to start NetlinkHandler", __FUNCTION__, __LINE__);
    return -1;
}

int PdmNetlinkManager::stop()
{
     if (m_handler->stop()) {
        PDM_LOG_CRITICAL("PdmNetlinkManager:%s line: %d Unable to stop NetlinkHandler", __FUNCTION__, __LINE__);
        return -1;
    }
    delete m_handler;
    m_handler = nullptr;
    return 0;
}
