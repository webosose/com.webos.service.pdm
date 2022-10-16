// Copyright (c) 2019-2022 LG Electronics, Inc.
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

#include "PdmNetlinkHandler.h"
#include "DeviceManager.h"
#include "PdmNetLinkCommand.h"

PdmNetlinkHandler::PdmNetlinkHandler(CommandManager *cmdManager)
{
    m_commandManager = cmdManager;
}

PdmNetlinkHandler::~PdmNetlinkHandler()
{

}

bool PdmNetlinkHandler::start()
{
    PDM_LOG_DEBUG("PdmNetlinkHandler:%s line: %d", __FUNCTION__, __LINE__);
    return this->startListener();
}

bool PdmNetlinkHandler::stop()
{
    PDM_LOG_DEBUG("PdmNetlinkHandler:%s line: %d", __FUNCTION__, __LINE__);
    return this->stopListener();
}

void PdmNetlinkHandler::onEvent(DeviceClass *deviceClassEvent)
{
    PdmNetLinkCommand *netLinkCmd = new (std::nothrow) PdmNetLinkCommand(deviceClassEvent);
    if(netLinkCmd)
        m_commandManager->sendCommand(netLinkCmd);
}
