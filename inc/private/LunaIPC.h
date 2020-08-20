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

#ifndef _LUNA_IPC_H_
#define _LUNA_IPC_H_

#include "PdmLunaService.h"

class CommandManager;

class LunaIPC
{
private:
    LunaIPC();
    LunaIPC(const LunaIPC& src) = delete;
    LunaIPC& operator=(const LunaIPC& rhs) = delete;
    LSHandle *mServiceHandle;
#ifdef WEBOS_SESSION
    LS::Handle *mServiceCPPHandle;
#endif
    PdmLunaService *mPdmService;
public:
    ~LunaIPC();
    bool init(GMainLoop *mainLoop,CommandManager *pCommandManager);
    bool deInit();
    static LunaIPC *getInstance();
    LSHandle *getLSHandle(void);
#ifdef WEBOS_SESSION
    LS::Handle *getLSCPPHandle(void);
    void getResumeDone();
#endif
    void notifyDeviceChange(int eventType,const int &eventID, std::string hubPortPath);
};


#endif /* _LUNA_IPC_H_ */
