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

#ifndef PDMLUNAHANDLER_H
#define PDMLUNAHANDLER_H

#include <functional>
#include <unordered_map>
#include <string>
#include <list>
#include <luna-service2/lunaservice.h>

#include "pbnjson.hpp"

using namespace std::placeholders;
using functionPtr =  std::function<bool (pbnjson::JValue &payload, LSMessage *message)>;
using fptrList = std::list<functionPtr>;
using fptrInfoMap = std::unordered_map<std::string, fptrList>;

const std::string GET_EXAMPLE = "getExample";
const std::string GET_STORAGEDEVICELIST = "getAttachedStorageDeviceList";
const std::string GET_NONSTORAGEDEVICELIST = "getAttachedNonStorageDeviceList";
const std::string GET_DEVICESTATUS = "getAttachedDeviceStatus";
const std::string GET_USBRESUMEDONE = "getUSBResumeDone";
const std::string GET_AUDIODEVICELIST = "getAttachedAudioDeviceList";
const std::string GET_AUDIOSUBDEVICELIST = "getAttachedAudioSubDeviceList";
const std::string GET_VIDEODEVICELIST = "getAttachedVideoDeviceList";
const std::string GET_VIDEOSUBDEVICELIST = "getAttachedVideoSubDeviceList";
const std::string GET_NETDEVICELIST = "getAttachedNetDeviceList";


class PdmLunaHandler {

private:
    fptrInfoMap mLunafptr;
    PdmLunaHandler();
public:
    ~PdmLunaHandler();
    static PdmLunaHandler *getInstance();
    bool registerLunaCallback(functionPtr funptr, const std::string &fName);
    bool getAttachedDeviceStatus(pbnjson::JValue &payload,LSMessage *message);
    bool getAttachedNonStorageDeviceList(pbnjson::JValue &payload , LSMessage *message);
    bool getAttachedStorageDeviceList (pbnjson::JValue &payload, LSMessage *message);
    bool getExampleAttachedStorageDeviceList (pbnjson::JValue &payload, LSMessage *message);
    bool getAttachedAudioDeviceList(pbnjson::JValue &payload, LSMessage *message);
    bool getAttachedAudioSubDeviceList(pbnjson::JValue &payload, LSMessage *message);
    bool getAttachedVideoDeviceList(pbnjson::JValue &payload, LSMessage *message);
    bool getAttachedVideoSubDeviceList(pbnjson::JValue &payload, LSMessage *message);
    bool getAttachedNetDeviceList(pbnjson::JValue &payload, LSMessage *message);
};
#endif //PDMLUNAHANDLER_H
