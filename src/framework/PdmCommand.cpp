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

#include "PdmCommand.h"
#include "DeviceManager.h"
#include "PdmLogUtils.h"

PdmCommand::PdmCommand(CommandType *cmdType,CommandResponseCallback cmdCallBack, void * message)
            : m_cmdType(cmdType)
            , m_cmdCallBack(cmdCallBack)
            , m_message(message)
{
    PDM_LOG_DEBUG("PdmCommand:%s line: %d", __FUNCTION__, __LINE__);
}

void PdmCommand::execute() {
    PDM_LOG_DEBUG("PdmCommand:%s line: %d", __FUNCTION__, __LINE__);
    CommandResponse cmdResponse;
    DeviceManager::getInstance()->HandlePdmCommand(m_cmdType,&cmdResponse);
    m_cmdCallBack(&cmdResponse,m_message);
}

PdmCommand::~PdmCommand(){
    PDM_LOG_DEBUG("PdmCommand:%s line: %d", __FUNCTION__, __LINE__);
    if(m_cmdType)
        delete m_cmdType;
    m_cmdType = nullptr;
    m_cmdCallBack = nullptr;
    m_message = nullptr;
}
