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

#ifndef PDMNETLINKCOMMAND_H_
#define PDMNETLINKCOMMAND_H_

#include "Command.h"
// #include "PdmNetlinkEvent.h"
#include "DeviceClass.h"

class PdmNetLinkCommand : public Command {

private:
    // PdmNetlinkEvent *m_netLinkEvent;
    DeviceClass *m_deviceClassEvent;
public:
    // PdmNetLinkCommand(PdmNetlinkEvent *event);
    // PdmNetLinkCommand(const PdmNetlinkEvent&) = delete;
    // PdmNetLinkCommand& operator=(const PdmNetlinkEvent&) = delete;
    PdmNetLinkCommand(DeviceClass *deviceClassEvent);
    PdmNetLinkCommand(const DeviceClass&) = delete;
    PdmNetLinkCommand& operator=(const DeviceClass&) = delete;
    ~PdmNetLinkCommand();
    void execute();

};

#endif /* PDMNETLINKCOMMAND_H_ */
