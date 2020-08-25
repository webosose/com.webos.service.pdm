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

#include "CommandManager.h"
#include "PdmLogUtils.h"

CommandManager::CommandManager() {
    m_cmdPool = new PdmThreadPool(1);
}

CommandManager::~CommandManager() {
    delete m_cmdPool;
    m_cmdPool = nullptr;
}

bool CommandManager::sendCommand(Command *cmd) {
    auto result = m_cmdPool->enqueue(&CommandManager::executeCommand, cmd);
    return true;
}

bool CommandManager::executeCommand(Command *cmd){
     PDM_LOG_DEBUG("CommandManager:%s line: %d", __FUNCTION__, __LINE__);
     cmd->execute();
     delete cmd;
     return true;
}
