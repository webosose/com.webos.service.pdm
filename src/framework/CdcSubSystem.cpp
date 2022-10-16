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
#include "CdcSubSystem.h"
#include "DeviceClassFactory.h"
#include "Common.h"
#include "PdmLogUtils.h"

using namespace PdmDevAttributes;

bool CdcSubSystem::mIsObjRegistered = CdcSubSystem::RegisterSubSystem();

CdcSubSystem::CdcSubSystem(std::unordered_map<std::string, std::string>& devPropMap)
	: mDevType("cdc"), DeviceClass(devPropMap)
{
	for (auto &prop : devPropMap)
		mDevPropMap[prop.first] = prop.second;
}

CdcSubSystem::~CdcSubSystem() {}

CdcSubSystem *CdcSubSystem::create(std::unordered_map<std::string, std::string> &devProMap)
{
    PDM_LOG_DEBUG("CdcSubSystem:%s line: %d", __FUNCTION__, __LINE__);
    bool canProcessEve = CdcSubSystem::canProcessEvent(devProMap);

    if (!canProcessEve)
        return nullptr;

    CdcSubSystem *ptr = new (std::nothrow) CdcSubSystem(devProMap);
    PDM_LOG_DEBUG("CdcSubSystem:%s line: %d CdcSubSystem object created", __FUNCTION__, __LINE__);
    return ptr;
}

std::string CdcSubSystem::getUsbModemId()
{
    return mDevPropMap[ID_USB_MODEM_DONGLE];
}

std::string CdcSubSystem::getIdUsbSerial()
{
    return mDevPropMap[ID_USB_SERIAL];
}

std::string CdcSubSystem::getUsbSerialId()
{
    return mDevPropMap[ID_USB_SERIAL];
}

std::string CdcSubSystem::getUsbSerialSubType()
{
    return mDevPropMap[USB_SERIAL_SUB_TYPE];
}

std::string CdcSubSystem::getNetIfIndex()
{
    return mDevPropMap[NET_IFIINDEX];
}

std::string CdcSubSystem::getNetLinkMode()
{
    return mDevPropMap[NET_LINK_MODE];
}

std::string CdcSubSystem::getNetDuplex()
{
    return mDevPropMap[NET_DUPLEX];
}

std::string CdcSubSystem::getNetAddress()
{
    return mDevPropMap[NET_ADDRESS];
}

std::string CdcSubSystem::getNetOperState()
{
    return mDevPropMap[NET_OPERSTATE];
}
