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

#ifndef _PDMNETLINKMANAGER_H
#define _PDMNETLINKMANAGER_H

#include "CommandManager.h"
#include "PdmNetlinkHandler.h"

class PdmNetlinkManager
{
private:

  PdmNetlinkHandler *m_handler;

  PdmNetlinkManager();
  PdmNetlinkManager(const PdmNetlinkManager& src) = delete;
  PdmNetlinkManager& operator=(const PdmNetlinkManager& rhs) = delete;

public:
    ~PdmNetlinkManager();
    int start(CommandManager *cmdManager);
    int stop();

    static PdmNetlinkManager *getInstance();
};

#endif //_PDMNETLINKMANAGER_H
