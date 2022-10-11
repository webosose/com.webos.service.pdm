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

#include "Common.h"
#include "PdmLunaHandler.h"

PdmLunaHandler *PdmLunaHandler::getInstance() {
    static PdmLunaHandler _instance;
    return &_instance;
}

PdmLunaHandler::PdmLunaHandler()
{
}
PdmLunaHandler::~PdmLunaHandler()
{
}

bool PdmLunaHandler::registerLunaCallback(functionPtr funptr, const std::string &fName)
{
    if(nullptr != funptr)
    {
        mLunafptr[fName].push_back(funptr);
    }
    return true;
}

bool PdmLunaHandler::getAttachedDeviceStatus(pbnjson::JValue &payload,LSMessage *message)
{
    for(auto deviceList : mLunafptr[GET_DEVICESTATUS])
    {
      deviceList(payload,message);
    }
    return true;
}

bool PdmLunaHandler::getAttachedNonStorageDeviceList(pbnjson::JValue &payload,LSMessage *message)
{

    pbnjson::JValue nonStorageDeviceList = pbnjson::Array();

    for(auto deviceNonStorageList : mLunafptr[GET_NONSTORAGEDEVICELIST])
    {
        deviceNonStorageList(nonStorageDeviceList,message);
    }

    payload.put("powerStatus", PdmDevAttributes::DEVICE_POWER_STATUS);
    payload.put("nonStorageDeviceList", nonStorageDeviceList);
    return true;
}

bool PdmLunaHandler::getAttachedStorageDeviceList(pbnjson::JValue &payload,LSMessage *message)
{

    pbnjson::JValue storageDeviceList = pbnjson::Array();

    for(auto deviceStorageList : mLunafptr[GET_STORAGEDEVICELIST])
    {
        deviceStorageList(storageDeviceList,message);
    }

    payload.put("powerStatus", PdmDevAttributes::DEVICE_POWER_STATUS);
    payload.put("storageDeviceList", storageDeviceList);
    return true;
}

bool PdmLunaHandler::getExampleAttachedStorageDeviceList(pbnjson::JValue &payload,LSMessage *message)
{

    pbnjson::JValue storageDeviceList = pbnjson::Array();

    for(auto deviceStorageList : mLunafptr[GET_EXAMPLE])
    {
        deviceStorageList(storageDeviceList,message);
    }

    payload.put("powerStatus", PdmDevAttributes::DEVICE_POWER_STATUS);
    payload.put("storageDeviceList", storageDeviceList);
    return true;
}

bool PdmLunaHandler::getAttachedAudioDeviceList(pbnjson::JValue &payload,LSMessage *message)
{

    pbnjson::JValue audioDeviceList = pbnjson::Array();

    for(auto deviceAudioList : mLunafptr[GET_AUDIODEVICELIST])
    {
        deviceAudioList(audioDeviceList,message);
    }

    payload.put("powerStatus", PdmDevAttributes::DEVICE_POWER_STATUS);
    payload.put("audioDeviceList", audioDeviceList);
    return true;
}

bool PdmLunaHandler::getAttachedAudioSubDeviceList(pbnjson::JValue &payload,LSMessage *message)
{

    pbnjson::JValue audioDeviceList = pbnjson::Array();

    for(auto deviceAudioList : mLunafptr[GET_AUDIOSUBDEVICELIST])
    {
        deviceAudioList(audioDeviceList,message);
    }

    payload.put("powerStatus", PdmDevAttributes::DEVICE_POWER_STATUS);
    payload.put("audioDeviceList", audioDeviceList);
    return true;
}

bool PdmLunaHandler::getAttachedVideoDeviceList(pbnjson::JValue &payload,LSMessage *message)
{
    pbnjson::JValue videoDeviceList = pbnjson::Array();

    for(auto deviceVideoList : mLunafptr[GET_VIDEODEVICELIST])
    {
        deviceVideoList(videoDeviceList,message);
    }

    payload.put("powerStatus", PdmDevAttributes::DEVICE_POWER_STATUS);
    payload.put("videoDeviceList", videoDeviceList);
    return true;
}

bool PdmLunaHandler::getAttachedVideoSubDeviceList(pbnjson::JValue &payload,LSMessage *message)
{
    pbnjson::JValue videoDeviceList = pbnjson::Array();

    for(auto deviceVideoList : mLunafptr[GET_VIDEOSUBDEVICELIST])
    {
        deviceVideoList(videoDeviceList, message);
    }

    payload.put("powerStatus", PdmDevAttributes::DEVICE_POWER_STATUS);
    payload.put("videoDeviceList", videoDeviceList);
    return true;
}

bool PdmLunaHandler::getAttachedNetDeviceList(pbnjson::JValue &payload,LSMessage *message)
{

    pbnjson::JValue netDeviceList = pbnjson::Array();

    for(auto deviceNetList : mLunafptr[GET_NETDEVICELIST])
    {
        deviceNetList(netDeviceList,message);
    }

    payload.put("powerStatus", PdmDevAttributes::DEVICE_POWER_STATUS);
    payload.put("netDeviceList", netDeviceList);
    return true;
}
