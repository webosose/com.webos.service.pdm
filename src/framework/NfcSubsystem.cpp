// Copyright (c) 2022 LG Electronics, Inc.
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

#include <functional>
#include "Common.h"
#include "NfcSubsystem.h"
#include "DeviceClassFactory.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

bool NfcSubsystem::mIsObjRegistered = NfcSubsystem::RegisterSubSystem();

NfcSubsystem::NfcSubsystem(std::unordered_map<std::string, std::string> &devPropMap)
    : mDevType("nfc"), DeviceClass(devPropMap)
{
    for (auto &prop : devPropMap)
        mDevPropMap[prop.first] = prop.second;
}

NfcSubsystem::~NfcSubsystem() {}

NfcSubsystem *NfcSubsystem::create(std::unordered_map<std::string, std::string> &devProMap)
{
    bool canProcessEve = NfcSubsystem::canProcessEvent(devProMap);

    if (!canProcessEve)
        return nullptr;

    NfcSubsystem *ptr = new (std::nothrow) NfcSubsystem(devProMap);
    PDM_LOG_DEBUG("NfcSubsystem:%s line: %d NfcSubsystem Object created", __FUNCTION__, __LINE__);
    return ptr;
}