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

#ifndef PDM_COMMAND_H_
#define PDM_COMMAND_H_

#include <functional>

#include "Command.h"
#include "CommandTypes.h"

using CommandResponseCallback = std::function< bool(CommandResponse*, void*) >;

class PdmCommand : public Command {

private:
    CommandType *m_cmdType;
    CommandResponseCallback m_cmdCallBack;
    void *m_message;

public:
    PdmCommand(CommandType *cmdType,CommandResponseCallback cmdCallBack, void * message);
    ~PdmCommand();
    void execute();
};



#endif /* PDM_COMMAND_H_ */
