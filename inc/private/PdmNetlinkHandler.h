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

#ifndef _PDMNETLINKHANDLER_H
#define _PDMNETLINKHANDLER_H

#include "CommandManager.h"
#include "PdmNetlinkListener.h"
#include "DeviceClass.h"

class PdmNetlinkHandler: public PdmNetlinkListener
{
private:
    CommandManager *m_commandManager;
public:
    explicit PdmNetlinkHandler(CommandManager *cmdManager);
    ~PdmNetlinkHandler();
    bool start();
    bool stop();
protected:
    void onEvent(DeviceClass *deviceClassEvent) override;
};

#endif //_PDMNETLINKMANAGER_H
