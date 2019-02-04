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

#include "DeviceHandler.h"
#include "PdmErrors.h"

void DeviceHandler::commandResponse(CommandResponse *cmdResponse, PdmDevStatus result)
{
    if(cmdResponse != nullptr)
    {
        bool returnValue = (result == PdmDevStatus::PDM_DEV_SUCCESS) ? true : false;
        cmdResponse->cmdResponse = pbnjson::Object();
        cmdResponse->cmdResponse.put("returnValue",returnValue);
        if (result != PdmDevStatus::PDM_DEV_SUCCESS) {
            cmdResponse->cmdResponse.put("errorCode", result);
            cmdResponse->cmdResponse.put("errorText", PdmErrors::mPdmErrorTextTable[result]);
        }
    }
}

bool DeviceHandler::HandlePluginEvent(int eventType) {
    return true;
}

