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

#include "LunaIPC.h"
#include "PdmLogUtils.h"

LunaIPC *LunaIPC::getInstance() {
    static LunaIPC _instance;
    return &_instance;
}

LunaIPC::LunaIPC(): mServiceHandle(nullptr)
                  , mPdmService(nullptr)
#ifdef WEBOS_SESSION
                  , mServiceCPPHandle(nullptr)
#endif
{

}

LunaIPC::~LunaIPC()
{
}

bool LunaIPC::init(GMainLoop *mainLoop,CommandManager *pCommandManager)
{
    PDM_LOG_DEBUG("LunaIPC: %s line: %d", __FUNCTION__, __LINE__);
    bool retVal = false;
    mPdmService = new(std::nothrow) PdmLunaService(pCommandManager);
    if(mPdmService == nullptr)
    {
        PDM_LOG_ERROR("LunaIPC: %s line: %d Failed allocation for PdmLunaService", __FUNCTION__, __LINE__);
        return retVal;
    }
    retVal = mPdmService->init(mainLoop);
    if(retVal)
    {
        mServiceHandle = mPdmService->get_LSHandle();
#ifdef WEBOS_SESSION
        mServiceCPPHandle = mPdmService->get_LSCPPHandle();
#endif
        PDM_LOG_DEBUG("LunaIPC: %s line: %d mServiceHandle =%p", __FUNCTION__, __LINE__,mServiceHandle);
    }
    return retVal;
}

bool LunaIPC::deInit()
{
    PDM_LOG_DEBUG("LunaIPC: %s line: %d", __FUNCTION__, __LINE__);
    bool retVal = false;
    retVal = mPdmService->deinit();
    if(retVal)
    {
        delete mPdmService;
        mPdmService = nullptr;
    }
    return retVal;
}

LSHandle *LunaIPC::getLSHandle()
{
    return mServiceHandle;
}

#ifdef WEBOS_SESSION
LS::Handle *LunaIPC::getLSCPPHandle()
{
    return mServiceCPPHandle;
}
void LunaIPC::getResumeDone() {
    mPdmService->notifyResumeDone();
}
#endif

void LunaIPC::notifyDeviceChange(int eventType, const int &eventID, std::string hubPortPath) {
    mPdmService->notifySubscribers(eventType,eventID,hubPortPath);
}
