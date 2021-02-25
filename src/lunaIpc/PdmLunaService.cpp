// Copyright (c) 2019-2021 LG Electronics, Inc.
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
#include <sys/mount.h>
#include <sys/types.h>
#include <sstream>
#include <sys/stat.h>
extern "C" {
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <mntent.h>
}

#include <sys/statfs.h>
#include "Common.h"
#include "CommandManager.h"
#include "JsonUtils.h"
#include "PdmCommand.h"
#include "PdmErrors.h"
#include "PdmGetExampleUtil.h"
#include "PdmLogUtils.h"
#include "PdmLunaHandler.h"
#include "PdmLunaService.h"
#include "SchemaValidationApi.h"
#include "PdmUtils.h"
#include "DiskFormat.h"

#include <luna-service2/lunaservice.hpp>
#include <luna-service2++/handle.hpp>
#include "LunaIPC.h"

#ifdef WEBOS_SESSION
#define DB8_KIND "com.webos.service.pdmhistory:1"

#define USER_MOUNT                            1

#define MOUNT_FILE_MODE        (S_IRWXU|S_IRGRP) //740
#define NON_STORAGE_FILE_MODE  (S_IRUSR| S_IWUSR|S_IRGRP) //640

std::map<std::string, std::string> PdmLunaService::m_sessionMap = {};
std::map<std::string, std::string> PdmLunaService::m_portDisplayMap = {};
const std::string DEVICE_CONNECTED_ICON_PATH = "/usr/share/physical-device-manager/usb_connect.png";
std::string PdmLunaService::m_sessionId = "host";
std::string PdmLunaService::m_deviceSetId = "";
std::string PdmLunaService::m_ejectedDeviceSetId = "";
std::string PdmLunaService::m_deviceType = "";
std::string PdmLunaService::m_requestedDrive = "";
bool PdmLunaService::isRequestForStorageDevice = false;

std::string PdmLunaService::avnUserId = "driver0";
std::string PdmLunaService::rselUserId = "guest0";
std::string PdmLunaService::rserUserId = "guest1";
#endif

const char* DeviceEventTable[] =
{
    PDM_EVENT_STORAGE_DEVICES,
    PDM_EVENT_NON_STORAGE_DEVICES,
    PDM_EVENT_ALL_ATTACHED_DEVICES,
    PDM_EVENT_ALL_ATTACHED_DEVICE_LIST
};

using namespace std;
using namespace std::placeholders;
using namespace PdmDevAttributes;

LSMethod PdmLunaService::pdm_methods[] = {
    {"getExample",                        PdmLunaService::_cbgetExample},
    {"getAttachedStorageDeviceList",    PdmLunaService::_cbgetAttachedStorageDeviceList},
    {"getAttachedNonStorageDeviceList",    PdmLunaService::_cbgetAttachedNonStorageDeviceList},
    {"getAttachedDeviceStatus",            PdmLunaService::_cbgetAttachedDeviceStatus},
    {"getSpaceInfo",                    PdmLunaService::_cbgetSpaceInfo},
    {"format",                            PdmLunaService::_cbformat},
    {"fsck",                            PdmLunaService::_cbfsck},
    {"eject",                            PdmLunaService::_cbeject},
    {"setVolumeLabel",                    PdmLunaService::_cbsetVolumeLabel},
    {"isWritableDrive",                    PdmLunaService::_cbisWritableDrive},
    {"umountAllDrive",                    PdmLunaService::_cbumountAllDrive},
    {"mountandFullFsck",                PdmLunaService::_cbmountandFullFsck},
#ifdef WEBOS_SESSION
    {"getAttachedAllDeviceList",        PdmLunaService::_cbgetAttachedAllDeviceList},
    {"getAttachedDeviceList",           PdmLunaService::_cbgetAttachedDeviceList},
#endif
    {NULL, NULL}
    };

#ifdef WEBOS_SESSION
    LSMethod PdmLunaService::pdm_dev_methods[] = {
        {"setDeviceForSession",             PdmLunaService::_cbSetDeviceForSession},
        {NULL, NULL}
        };
#endif

PdmLunaService::PdmLunaService(CommandManager *cmdManager)
    : mServiceHandle(nullptr)
    , mCommandManager(cmdManager)
#ifdef WEBOS_SESSION
    , mServiceCPPHandle(nullptr)
    , replyMsg(nullptr)
    , isGetSpaceInfoRequest(false)
#endif
{

}

PdmLunaService::~PdmLunaService()
{

}

void PdmLunaService::LSErrorPrintAndFree(LSError *ptrLSError) {
    if (ptrLSError != NULL) {
        LSErrorPrint(ptrLSError, stderr);
        LSErrorFree(ptrLSError);
    }
}

void PdmLunaService::appendErrorResponse(pbnjson::JValue &payload, int errorCode, std::string errorText)
{
    payload.put("returnValue", false);
    payload.put("errorCode", errorCode);
    payload.put("errorText", errorText.c_str());
}

bool PdmLunaService::init(GMainLoop *mainLoop) {
    PDM_LOG_DEBUG("PdmLunaService::init");

#ifdef WEBOS_SESSION
    mServiceCPPHandle = new LS::Handle(PDM_SERVICE_NAME);
    mServiceHandle = mServiceCPPHandle->get();
#endif

    if (PdmLunaServiceRegister(PDM_SERVICE_NAME, mainLoop, &mServiceHandle) == false) {
        PDM_LOG_ERROR("com.webos.service.pdm service registration failed");
        return false;
    }
    PDM_LOG_DEBUG("mServiceHandle =%p", mServiceHandle);

#ifdef WEBOS_SESSION
    if (!queryForSession())
    {
        PDM_LOG_ERROR("PdmLunaService::%s:%d sessionId fetching failed", __FUNCTION__, __LINE__);
    }
#endif

    return true;
}

bool PdmLunaService::PdmLunaServiceRegister(const char *srvcname, GMainLoop *mainLoop, LSHandle **msvcHandle) {

    bool bRetVal = false;
    LSError error;
    LSErrorInit(&error);

    PDM_LOG_DEBUG("PdmLunaService::PdmLunaServiceRegister");

#ifndef WEBOS_SESSION
    //Register the service
    bRetVal = LSRegister(srvcname, msvcHandle, &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
#endif

    //register category
    bRetVal = LSRegisterCategory(*msvcHandle, "/", pdm_methods, nullptr, nullptr, &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);

    bRetVal =  LSCategorySetData(*msvcHandle,  "/", this, &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);

#ifdef WEBOS_SESSION
    bRetVal = LSRegisterCategory(*msvcHandle, "/dev", pdm_dev_methods, nullptr, nullptr, &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);

    bRetVal =  LSCategorySetData(*msvcHandle,  "/dev", this, &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
#endif

    //Gmain attach
    bRetVal = LSGmainAttach(*msvcHandle, mainLoop, &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);

    return bRetVal;
}

bool PdmLunaService::deinit()
{
    bool bRetVal = false;
    LSError error;
    LSErrorInit(&error);

    bRetVal = LSUnregister(mServiceHandle, &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
    return bRetVal;
}

LSHandle *PdmLunaService::get_LSHandle()
{
    return mServiceHandle;
}

#ifdef WEBOS_SESSION
LS::Handle *PdmLunaService::get_LSCPPHandle()
{
    return mServiceCPPHandle;
}
#endif

pbnjson::JValue PdmLunaService::createJsonGetAttachedDeviceStatus(LSMessage *message )
{
    PDM_LOG_DEBUG("PdmLunaService::createJsonGetAttachedDeviceStatus");
    pbnjson::JValue payload = pbnjson::Object();
    pbnjson::JValue deviceStatusList = pbnjson::Array();

    if(PdmLunaHandler::getInstance()->getAttachedDeviceStatus(deviceStatusList,message))
    {
        payload.put("deviceStatusList", deviceStatusList);
    }else{
        payload.put("errorCode", 0);
        payload.put("errorText", "Failure");
    }
    payload.put("returnValue", true);
    return payload;
}

pbnjson::JValue PdmLunaService::createJsonGetAttachedStorageDeviceList(LSMessage *message)
{
    pbnjson::JValue payload = pbnjson::Object();
    PDM_LOG_DEBUG("PdmLunaService::createJsonGetAttachedStorageDeviceList");

    if(PdmLunaHandler::getInstance()->getAttachedStorageDeviceList(payload,message))
    {
        payload.put("returnValue", true);
    }else{
        appendErrorResponse(payload, PdmPayload::PDM_RESPONSE_FAILURE, PdmErrors::mPdmErrorTextTable[PdmPayload::PDM_RESPONSE_FAILURE]);
    }
    return payload;
}


pbnjson::JValue PdmLunaService::createJsonGetAttachedNonStorageDeviceList(LSMessage *message)
{
    pbnjson::JValue payload = pbnjson::Object();

    const char* payloadMsg = LSMessageGetPayload(message);
    std::string category;
    if (payloadMsg) {
        pbnjson::JValue list = pbnjson::JDomParser::fromString(payloadMsg);
        category = list["category"].asString();
    }
    PDM_LOG_DEBUG("PdmLunaService::createJsonGetAttachedNonStorageDeviceList category as %s",category.c_str());

    if(category.empty()) {
        if(PdmLunaHandler::getInstance()->getAttachedNonStorageDeviceList(payload,message)) {
            payload.put("returnValue", true);
            }else{
            appendErrorResponse(payload, PdmPayload::PDM_RESPONSE_FAILURE, PdmErrors::mPdmErrorTextTable[PdmPayload::PDM_RESPONSE_FAILURE]);
            }
    }else if(category.compare("Audio") == 0) {
        if(PdmLunaHandler::getInstance()->getAttachedAudioDeviceList(payload,message))
        {
            payload.put("returnValue", true);
        }else{
            appendErrorResponse(payload, PdmPayload::PDM_RESPONSE_FAILURE, PdmErrors::mPdmErrorTextTable[PdmPayload::PDM_RESPONSE_FAILURE]);
        }
    }else if(category.compare("Video") == 0) {
        if(PdmLunaHandler::getInstance()->getAttachedVideoDeviceList(payload,message))
        {
            payload.put("returnValue", true);
        }else{
            appendErrorResponse(payload, PdmPayload::PDM_RESPONSE_FAILURE, PdmErrors::mPdmErrorTextTable[PdmPayload::PDM_RESPONSE_FAILURE]);
        }
    }else if(category.compare("Net") == 0) {
        if(PdmLunaHandler::getInstance()->getAttachedNetDeviceList(payload,message))
        {
            payload.put("returnValue", true);
        }else {
            appendErrorResponse(payload, PdmPayload::PDM_RESPONSE_FAILURE, PdmErrors::mPdmErrorTextTable[PdmPayload::PDM_RESPONSE_FAILURE]);
        }
    }else {
        appendErrorResponse(payload, PdmPayload::PDM_RESPOSE_CATEGORY_MISMATCH, PdmErrors::mPdmErrorTextTable[PdmPayload::PDM_RESPOSE_CATEGORY_MISMATCH]);
    }
    return payload;
}

bool PdmLunaService::cbGetExample(LSHandle *sh, LSMessage *message)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    bool bRetVal;
    LSError error;
    LSErrorInit(&error);

    pbnjson::JValue replyPayload = pbnjson::Object();

    PdmGetExampleUtil getExampleUtilObj;
    if(getExampleUtilObj.printGetExampleLogs())
    {
        replyPayload.put("returnValue", true);
    }else{
        appendErrorResponse(replyPayload, PdmPayload::PDM_RESPONSE_FAILURE, PdmErrors::mPdmErrorTextTable[PdmPayload::PDM_RESPONSE_FAILURE]);
    }

    bRetVal  =  LSMessageReply (sh,  message,  replyPayload.stringify(NULL).c_str() ,  &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
    return true;
}

bool PdmLunaService::cbGetAttachedStorageDeviceList(LSHandle *sh, LSMessage *message)
{
    bool bRetVal;
    LSError error;
    LSErrorInit(&error);
    bool subscribed = false;
#ifdef WEBOS_SESSION
    replyMsg = message;
    pbnjson::JValue payload = pbnjson::Object();

    std::string sessionId = LSMessageGetSessionId(message);
    m_sessionId = sessionId;
    std::string deviceSetId = "";
    queryForSession();
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d sessionId: %s", __FUNCTION__, __LINE__, sessionId.c_str());

    if (sessionId != "host")
    {
        auto sessionPair = std::find_if(std::begin(m_sessionMap), std::end(m_sessionMap), [&](const std::pair<std::string, std::string> &pair)
        {
            return (pair.second == sessionId);
        });

        if (sessionPair != std::end(m_sessionMap))
        {
            deviceSetId = sessionPair->first;
        }
    }
    bRetVal = getDevicesFromDB("USB_STORAGE", sessionId);
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d payload: %s", __FUNCTION__, __LINE__, payload.stringify().c_str());
    if (LSMessageIsSubscription(message))
    {
        if (sessionId != "host")
        {
            if (deviceSetId == "AVN")
            {
                PDM_LOG_DEBUG("PdmLunaService::%s line:%d", __FUNCTION__, __LINE__);
                subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_DEVICES_AVN, message);
            }
            else if (deviceSetId == "RSE-L")
            {
                PDM_LOG_DEBUG("PdmLunaService::%s line:%d", __FUNCTION__, __LINE__);
                subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_DEVICES_RSE_L, message);
            }
            else if (deviceSetId == "RSE-R")
            {
                PDM_LOG_DEBUG("PdmLunaService::%s line:%d", __FUNCTION__, __LINE__);
                subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_DEVICES_RSE_R, message);
            }
        }
        else
        {
            PDM_LOG_DEBUG("PdmLunaService::%s line:%d", __FUNCTION__, __LINE__);
            subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_STORAGE_DEVICES, message);
        }
    }
#else

    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_VALIDATE_ATTACH_DEVICE_LIST);
    pbnjson::JValue payload = createJsonGetAttachedStorageDeviceList(message);

    PDM_LOG_DEBUG("PdmLunaService::cbgetAttachedStorageDeviceList");

    if (LSMessageIsSubscription(message))
    {
        subscribed = subscriptionAdd(sh, PDM_EVENT_STORAGE_DEVICES, message);
    }

    payload.put("returnValue", subscribed);

    bRetVal  =  LSMessageReply (sh,  message,  payload.stringify(NULL).c_str() ,  &error);
#endif
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
    return true;
}

bool PdmLunaService::cbGetAttachedNonStorageDeviceList(LSHandle *sh, LSMessage *message)
{
    bool bRetVal;
    LSError error;
    LSErrorInit(&error);
    bool subscribed = false;
#ifdef WEBOS_SESSION
    replyMsg = message;

    std::string sessionId = LSMessageGetSessionId(message);
    m_sessionId = sessionId;
    std::string deviceSetId = "";
    queryForSession();
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d sessionId: %s", __FUNCTION__, __LINE__, sessionId.c_str());

    if (sessionId == "host")
        VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_VALIDATE_NON_STORAGE_ATTACH_DEVICE_LIST);

    const char* payloadMsg = LSMessageGetPayload(message);
    pbnjson::JValue payload = pbnjson::Object();
    std::string category;
    if (payloadMsg) {
        pbnjson::JValue list = pbnjson::JDomParser::fromString(payloadMsg);
        category = list["category"].asString();
    }
    PDM_LOG_DEBUG("PdmLunaService::createJsonGetAttachedNonStorageDeviceList category as %s",category.c_str());
    if(!category.empty() && (category.compare("Net") != 0) &&
       (category.compare("Audio") != 0) && (category.compare("Video") != 0)) {
        appendErrorResponse(payload, PdmPayload::PDM_RESPOSE_CATEGORY_MISMATCH, PdmErrors::mPdmErrorTextTable[PdmPayload::PDM_RESPOSE_CATEGORY_MISMATCH]);
        bRetVal  =  LSMessageReply (sh,  message,  payload.stringify(NULL).c_str() ,  &error);
        LSERROR_CHECK_AND_PRINT(bRetVal, error);
        return true;
    }

    if (sessionId != "host")
    {
        auto sessionPair = std::find_if(std::begin(m_sessionMap), std::end(m_sessionMap), [&](const std::pair<std::string, std::string> &pair)
        {
            return (pair.second == sessionId);
        });

        if (sessionPair != std::end(m_sessionMap))
        {
            deviceSetId = sessionPair->first;
        }
    }
    bRetVal = getDevicesFromDB("USB_NONSTORAGE", sessionId);

    PDM_LOG_DEBUG("PdmLunaService::%s line:%d payload: %s", __FUNCTION__, __LINE__, payload.stringify().c_str());
    if (LSMessageIsSubscription(message))
    {
        if (sessionId != "host")
        {
            if (deviceSetId == "AVN")
            {
                PDM_LOG_DEBUG("PdmLunaService::%s line:%d subscriptionKey: %s", __FUNCTION__, __LINE__, PDM_EVENT_AUTO_NON_STORAGE_DEVICES_AVN);
                subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_NON_STORAGE_DEVICES_AVN, message);
            }
            else if (deviceSetId == "RSE-L")
            {
                PDM_LOG_DEBUG("PdmLunaService::%s line:%d subscriptionKey: %s", __FUNCTION__, __LINE__, PDM_EVENT_AUTO_NON_STORAGE_DEVICES_RSE_L);
                subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_NON_STORAGE_DEVICES_RSE_L, message);
            }
            else if (deviceSetId == "RSE-R")
            {
                PDM_LOG_DEBUG("PdmLunaService::%s line:%d subscriptionKey: %s", __FUNCTION__, __LINE__, PDM_EVENT_AUTO_NON_STORAGE_DEVICES_RSE_R);
                subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_NON_STORAGE_DEVICES_RSE_R, message);
            }
        }
        else
        {
            PDM_LOG_DEBUG("PdmLunaService::%s line:%d subscriptionKey: %s", __FUNCTION__, __LINE__, PDM_EVENT_AUTO_NON_STORAGE_DEVICES);
            subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_NON_STORAGE_DEVICES, message);
        }
    }
#else
    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_VALIDATE_NON_STORAGE_ATTACH_DEVICE_LIST);
    pbnjson::JValue payload = createJsonGetAttachedNonStorageDeviceList(message);
    PDM_LOG_DEBUG("PdmLunaService::cbgetAttachedNonStorageDeviceList");
    if (LSMessageIsSubscription(message))
    {
        subscribed = subscriptionAdd(sh, PDM_EVENT_NON_STORAGE_DEVICES, message);
    }

    payload.put("returnValue", subscribed);

    bRetVal  =  LSMessageReply (sh,  message,  payload.stringify(NULL).c_str() ,  &error);
#endif
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
    return true;
}

bool PdmLunaService::cbGetAttachedDeviceStatus(LSHandle *sh, LSMessage *message)
{
    bool bRetVal;
    LSError error;
    LSErrorInit(&error);
    bool subscribed = false;
    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_VALIDATE_ATTACH_DEVICE_LIST);
    pbnjson::JValue payload = createJsonGetAttachedDeviceStatus(message);

    PDM_LOG_DEBUG("PdmLunaService::cbgetAttachedDeviceStatus");

    if (LSMessageIsSubscription(message))
    {
        subscribed = subscriptionAdd(sh, PDM_EVENT_ALL_ATTACHED_DEVICES, message);
    }

    payload.put("returnValue", subscribed);

    bRetVal  =  LSMessageReply (sh,  message,  payload.stringify(NULL).c_str() ,  &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
    return true;
}

bool PdmLunaService::commandReply(CommandResponse *cmdRes, void *msg)
{
    bool bRetVal = false;
    LSError error;
    LSErrorInit(&error);
    LSMessage *message = static_cast<LSMessage*>(msg);
    bRetVal = LSMessageRespond(message,cmdRes->cmdResponse.stringify(NULL).c_str(), &error );
    LSMessageUnref(message);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
    return bRetVal;
}

bool PdmLunaService::cbGetSpaceInfo(LSHandle *sh, LSMessage *message)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));
    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_GET_SPACE_INFO_VALIDATE_DRIVE_NAME);
    const char* payloadMsg = LSMessageGetPayload(message);

    if(!payloadMsg) {
         PDM_LOG_ERROR("PdmLunaService:%s line: %d payloadMsg is empty ", __FUNCTION__, __LINE__);
         return true;
    }
    pbnjson::JValue list = pbnjson::JDomParser::fromString(payloadMsg);

#ifdef WEBOS_SESSION
    isGetSpaceInfoRequest = true;
    std::string driveName = list["driveName"].asString();
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d driveName: %s", __FUNCTION__, __LINE__, driveName.c_str());
    m_requestedDrive = driveName;
    LSError lserror;
    LSErrorInit(&lserror);
    LSMessageRef(message);
    replyMsg = message;
    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue request;
    request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                {"where", pbnjson::JArray{{{"prop", "deviceType"}, {"op", "="}, {"val","USB_STORAGE"}}}}};

    find_query.put("query", request);
    if (LSCallOneReply(sh,"luna://com.webos.service.db/find",
                         find_query.stringify().c_str(), cbFindDriveName, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

#else

    SpaceInfoCommand *spaceCmd = new (std::nothrow) SpaceInfoCommand;
    if(!spaceCmd) {
         PDM_LOG_ERROR("PdmLunaService:%s line: %d SpaceInfoCommand ", __FUNCTION__, __LINE__);
         return true;
    }
    spaceCmd->directCheck = list["directCheck"].asBool();
    spaceCmd->driveName = list["driveName"].asString();
    LSMessageRef(message);
    PdmCommand *cmdSpace = new (std::nothrow) PdmCommand(reinterpret_cast<CommandType*>(spaceCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );

    if(cmdSpace){
        mCommandManager->sendCommand(cmdSpace);
    } else {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d cmdSpace creation failed ", __FUNCTION__, __LINE__);
    }
#endif
    return true;
}

bool PdmLunaService::cbFormat(LSHandle *sh, LSMessage *message)
{

    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));
    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_FORMAT_VALIDATE_DRIVE_NAME);
    const char* payload = LSMessageGetPayload(message);

    if (payload) {
        pbnjson::JValue list = pbnjson::JDomParser::fromString(payload);

        FormatCommand *formatCmd = new FormatCommand;
        if(!formatCmd) {
            return true;
        }

        formatCmd->driveName = list["driveName"].asString();
        formatCmd->fsType = list["fsType"].asString();
        formatCmd->volumeLabel = list["volumeLabel"].asString();
        LSMessageRef(message);
        PdmCommand *cmdFormat = new PdmCommand(reinterpret_cast<CommandType*>(formatCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );
        mCommandManager->sendCommand(cmdFormat);
    }

    return true;
}

bool PdmLunaService::cbFsck(LSHandle *sh, LSMessage *message)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));
    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_VALIDATE_DRIVE_NAME);
    const char* payload = LSMessageGetPayload(message);

    if (payload) {
        pbnjson::JValue list = pbnjson::JDomParser::fromString(payload);

        FsckCommand *fsckCmd = new FsckCommand;
        if(!fsckCmd)
            return true;

        fsckCmd->driveName = list["driveName"].asString();
        LSMessageRef(message);
        PdmCommand *cmdFsck = new PdmCommand(reinterpret_cast<CommandType*>(fsckCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );
        mCommandManager->sendCommand(cmdFsck);
    }
    return true;
}

bool PdmLunaService::cbEject(LSHandle *sh, LSMessage *message)
{
    LSError error;
    LSErrorInit(&error);

    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));
    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_VALIDATE_DEVICE_NUMBER);
    const char* payload = LSMessageGetPayload(message);

    pbnjson::JValue root = pbnjson::JDomParser::fromString(payload);

    if(root.isNull()) {
        // handle error -> Couldnt parse json
        return true;
    }
#ifdef WEBOS_SESSION
    LSError lserror;
    LSErrorInit(&lserror);
    LSMessageRef(message);
    replyMsg = message;
    int deviceNum = root["deviceNum"].asNumber<int>();
    queryForSession();
    findDevice(sh, deviceNum);
#else
    EjectCommand *ejectCmd = new EjectCommand;
    if(!ejectCmd)
        return true;

    ejectCmd->deviceNumber = root["deviceNum"].asNumber<int>();
    LSMessageRef(message);
    PdmCommand *cmdeject = new PdmCommand(reinterpret_cast<CommandType*>(ejectCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );
    mCommandManager->sendCommand(cmdeject);
#endif
    return true;
}

#ifdef WEBOS_SESSION
void PdmLunaService::findDevice(LSHandle * sh,int deviceNum)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d devicenum: %d", __FUNCTION__, __LINE__, deviceNum);
    LSError lserror;
    LSErrorInit(&lserror);
    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue request;
    request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                    {"where", pbnjson::JArray{{{"prop", "deviceNum"}, {"op", "="}, {"val",deviceNum}}}}};

    find_query.put("query", request);
    if (LSCallOneReply(sh,"luna://com.webos.service.db/find",
                             find_query.stringify().c_str(), cbEjectDevice, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

bool PdmLunaService::cbEjectDevice(LSHandle * sh, LSMessage * message, void * user_data)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    bool deviceFound = true;
    const char* payload = LSMessageGetPayload(message);
    pbnjson::JValue json = pbnjson::Object();
    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    if(request.isNull() || (!request["returnValue"].asBool())){
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Db8Response is empty ", __FUNCTION__, __LINE__);
        deviceFound = false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if(nullptr == object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d PdmLunaService obj is NULL", __FUNCTION__, __LINE__);
        return false;
    }
    LSMessage* ejectFailReplyMsg = object->getReplyMsg();
    if (!ejectFailReplyMsg) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d ejectFailReplyMsg is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue resultArray = request["results"];
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d list:%s", __FUNCTION__, __LINE__, resultArray.stringify().c_str());
    if(resultArray.isArray()) {
        if(resultArray.arraySize() == 0) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d No device Info in DB ", __FUNCTION__, __LINE__);
            deviceFound = false;
        }
        if((deviceFound) && (!object->ejectDevice(resultArray,object))) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d unable to delete from db", __FUNCTION__, __LINE__);
        }
    }
    json.put("returnValue", false);
    json.put("errorText", "No device found");
    if(!deviceFound) {
        if(!LSMessageReply( sh, ejectFailReplyMsg, json.stringify(NULL).c_str(), &lserror)) {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
            LSMessageUnref(ejectFailReplyMsg);
        }
    }
    return true;
}
bool PdmLunaService::ejectDevice(pbnjson::JValue list, PdmLunaService* object)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool allPartitionUnmounted = false;

    //Add kind to the object
    list.put("_kind", DB8_KIND);
    pbnjson::JValue mergeput_query = pbnjson::Object();
    pbnjson::JValue query;
    pbnjson::JValue props = pbnjson::Object();
    std::string hubPortPath = list[0]["hubPortPath"].asString();
    std::string deviceSetId = list[0]["deviceSetId"].asString();
    std::string errorReason = list[0]["errorReason"].asString();
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d hubPortPath: %s deviceSetId:%s errorReason:%s ", __FUNCTION__, __LINE__, hubPortPath.c_str(),deviceSetId.c_str(),errorReason.c_str());

    LSMessage* ejectPassReplyMsg = object->getReplyMsg();
    if (!ejectPassReplyMsg) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d ejectPassReplyMsg is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    if(errorReason == "Ejected"){
        pbnjson::JValue json = pbnjson::Object();
        json.put("returnValue", true);
        json.put("error Text", "Already ejected");

        if(!LSMessageReply( mServiceHandle, ejectPassReplyMsg, json.stringify(NULL).c_str(), &lserror)) {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
            LSMessageUnref(ejectPassReplyMsg);
        }
        return true;
    }
    m_ejectedDeviceSetId = deviceSetId;
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d hubPortPath: %s deviceSetId:%s", __FUNCTION__, __LINE__, hubPortPath.c_str(),deviceSetId.c_str());
    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                               {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};

    if (list[0]["deviceType"] == "USB_STORAGE")
    {
        for(ssize_t idx = 0; idx < list[0]["storageDriveList"].arraySize() ; idx++) {
            std::string mountName = list[0]["storageDriveList"][idx]["mountName"].asString();
            PDM_LOG_DEBUG("PdmLunaService::%s line:%d mountName: %s", __FUNCTION__, __LINE__, mountName.c_str());
            allPartitionUnmounted = false;
            if(umount(mountName)) {
                int ret = PdmUtils::removeDirRecursive(mountName);
                if(ret) {
                    std::string mountName;
                    if(deviceSetId == "RSE-L"){
                        mountName = "/tmp/users/" + rselUserId +"/usb";
                        list[0]["storageDriveList"][idx].put("mountName",mountName);
                    } else if (deviceSetId == "RSE-R") {
                        mountName = "/tmp/users/" + rserUserId +"/usb";
                        list[0]["storageDriveList"][idx].put("mountName",mountName);
                    } else if(deviceSetId == "AVN") {
                        mountName = "/tmp/users/" + avnUserId +"/usb";
                        list[0]["storageDriveList"][idx].put("mountName",mountName);
                    }
                }
                list[0]["storageDriveList"][idx].put("isMounted", false);
                allPartitionUnmounted = true;
            }
        }
        if(allPartitionUnmounted) {
            list[0].put("errorReason","Ejected");
        }
    }
    mergeput_query.put("query", query);
    mergeput_query.put("props", list[0]);

    PDM_LOG_DEBUG("PdmLunaService::%s line:%d MergePutPayload: %s", __FUNCTION__, __LINE__, mergeput_query.stringify().c_str());
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/mergePut",
                       mergeput_query.stringify().c_str(), cbDb8Response, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("Store device info to History table call failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        return false;
    }
    pbnjson::JValue json = pbnjson::Object();
    json.put("returnValue", true);
    if(!allPartitionUnmounted) {
        json.put("returnValue", false);
        json.put("error Text", "Eject failed");
        return false;
    }
    if(!LSMessageReply( mServiceHandle, ejectPassReplyMsg, json.stringify(NULL).c_str(), &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        LSMessageUnref(ejectPassReplyMsg);
    }
    updateDeviceAlldeviceList(); //update ejected device Info in getAttachedAlldeviceList
    updateDeviceList(deviceSetId); //update ejected device Info in getAttachedDeviceList
    updateStorageDeviceList(deviceSetId); ////update ejected device Info in getAttachedStoragedeviceList
    return true;
}

void PdmLunaService::updateDeviceAlldeviceList()
{
    bool bRetVal;
    LSError error;
    LSErrorInit(&error);
    pbnjson::JValue payload;
    payload = createJsonGetAttachedAllDeviceList(nullptr);
    bRetVal = LSSubscriptionReply(mServiceHandle,PDM_EVENT_ALL_ATTACHED_DEVICE_LIST, payload.stringify(NULL).c_str(), &error);
}

void PdmLunaService::updateStorageDeviceList(std::string deviceSetId)
{
    pbnjson::JValue find_query = pbnjson::Object();
    LSError lserror;
    LSErrorInit(&lserror);
    pbnjson::JValue request;
    request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                              {"where", pbnjson::JArray{{{"prop", "deviceSetId"}, {"op", "="}, {"val", deviceSetId.c_str()}}}}};
    find_query.put("query", request);
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/find",
                      find_query.stringify().c_str(), cbUpdateStorageDeviceListResponse, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

void PdmLunaService::updateDeviceList(std::string deviceSetId)
{
    pbnjson::JValue find_query = pbnjson::Object();
    LSError lserror;
    LSErrorInit(&lserror);
    pbnjson::JValue request;
    request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                              {"where", pbnjson::JArray{{{"prop", "deviceSetId"}, {"op", "="}, {"val", deviceSetId.c_str()}}}}};
    find_query.put("query", request);
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/find",
                      find_query.stringify().c_str(), cbUpdateDeviceListResponse, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

bool PdmLunaService::cbUpdateDeviceListResponse(LSHandle * sh, LSMessage * message, void * user_data)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    const char* payload = LSMessageGetPayload(message);
    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    if(request.isNull() || (!request["returnValue"].asBool()))
    {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Db8Response is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if (!object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d object is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue deviceInfoArray = pbnjson::Array();
    pbnjson::JValue resultArray = request["results"];
    if(resultArray.isArray()) {
        if(resultArray.arraySize() == 0) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d No device Info in DB ", __FUNCTION__, __LINE__);
        }
    }
    pbnjson::JValue storageDeviceList =object->getStorageDevicePayload(resultArray);
    deviceInfoArray.put(0, storageDeviceList);
    pbnjson::JValue nonStorageDeviceList =object->getNonStorageDevicePayload(resultArray);
    deviceInfoArray.put(1, nonStorageDeviceList);
    pbnjson::JValue json = pbnjson::Object();
    json.put("returnValue", true);
    json.put("deviceListInfo", deviceInfoArray);
    object->notifyAllDeviceToDisplay(m_ejectedDeviceSetId, json);
    return true;
}

bool PdmLunaService::cbUpdateStorageDeviceListResponse(LSHandle * sh, LSMessage * message, void * user_data)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    const char* payload = LSMessageGetPayload(message);
    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    if(request.isNull() || (!request["returnValue"].asBool()))
    {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Db8Response is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if (!object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d object is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue deviceInfoArray = pbnjson::Array();
    pbnjson::JValue resultArray = request["results"];
    if(resultArray.isArray()) {
        if(resultArray.arraySize() == 0) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d No device Info in DB ", __FUNCTION__, __LINE__);
        }
    }
    pbnjson::JValue storageDeviceList =object->getStorageDevicePayload(resultArray);
    deviceInfoArray.put(0, storageDeviceList);
    pbnjson::JValue json = pbnjson::Object();
    json.put("returnValue", true);
    json.put("deviceListInfo", deviceInfoArray);
    object->notifyToDisplay(json, m_ejectedDeviceSetId, "USB_STORAGE");
    return true;
}

bool PdmLunaService::umount(std::string mountName)
{
    bool retValue = true;
    int umountFlags = MNT_FORCE;

    if(1)
        umountFlags |= MNT_DETACH;

    int res = umount2(mountName.c_str(), umountFlags);
    if(res != 0) {
        PDM_LOG_ERROR("PdmFs:%s line: %d Umount Failed :%s", __FUNCTION__, __LINE__, strerror(errno));
        retValue = false;
    }
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d mountName: %s retValue:%d", __FUNCTION__, __LINE__, mountName.c_str(), retValue);
    return retValue;
}
#endif

bool PdmLunaService::cbSetVolumeLabel(LSHandle *sh, LSMessage *message)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));

    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_VALIDATE_DRIVE_NAME_VOLUME_LABEL);
    const char* payload = LSMessageGetPayload(message);

    if (payload) {
        pbnjson::JValue list = pbnjson::JDomParser::fromString(payload);

        VolumeLabelCommand *labelCmd = new VolumeLabelCommand;
        if(!labelCmd)
            return true;

        labelCmd->driveName = list["driveName"].asString();
        labelCmd->volumeLabel = list["volumeLabel"].asString();
        LSMessageRef(message);
        PdmCommand *cmdVolumeLabel = new PdmCommand(reinterpret_cast<CommandType*>(labelCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );
        mCommandManager->sendCommand(cmdVolumeLabel);

    }

    return true;
}

bool PdmLunaService::cbIsWritableDrive(LSHandle *sh, LSMessage *message)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));

    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_VALIDATE_DRIVE_NAME);
    const char* payload = LSMessageGetPayload(message);

    if (payload) {
        pbnjson::JValue list = pbnjson::JDomParser::fromString(payload);
#ifdef WEBOS_SESSION
        isGetSpaceInfoRequest = false;
        std::string driveName = list["driveName"].asString();
        PDM_LOG_DEBUG("PdmLunaService:%s line: %d driveName: %s", __FUNCTION__, __LINE__, driveName.c_str());
        m_requestedDrive = driveName;
        LSError lserror;
        LSErrorInit(&lserror);
        LSMessageRef(message);
        replyMsg = message;
        pbnjson::JValue find_query = pbnjson::Object();
        pbnjson::JValue request;
        request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                    {"where", pbnjson::JArray{{{"prop", "deviceType"}, {"op", "="}, {"val","USB_STORAGE"}}}}};

        find_query.put("query", request);
        if (LSCallOneReply(sh,"luna://com.webos.service.db/find",
                             find_query.stringify().c_str(), cbFindDriveName, this, NULL, &lserror) == false) {
            PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }
    }

#else

        IsWritableCommand *writableCmd = new IsWritableCommand;
        if(!writableCmd)
            return true;

        writableCmd->driveName = list["driveName"].asString();
        LSMessageRef(message);
        PdmCommand *cmdIsWritable = new PdmCommand(reinterpret_cast<CommandType*>(writableCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );
        mCommandManager->sendCommand(cmdIsWritable);

    }
#endif
    return true;
}

#ifdef WEBOS_SESSION
bool PdmLunaService::cbFindDriveName(LSHandle * sh, LSMessage * message, void * user_data)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    bool driveFound = false;
    bool isWritable = false;
    const char* payload = LSMessageGetPayload(message);
    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if(nullptr == object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d PdmLunaService obj is NULL", __FUNCTION__, __LINE__);
        return false;
    }
    LSMessage* requestedDriveReplyMsg = object->getReplyMsg();
    if (!requestedDriveReplyMsg) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d requestedDriveReplyMsg is empty ", __FUNCTION__, __LINE__);
        return false;
    }

    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    if(request.isNull() || (!request["returnValue"].asBool())){
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Db8Response is empty ", __FUNCTION__, __LINE__);
    }
    pbnjson::JValue json = pbnjson::Object();
    pbnjson::JValue resultArray = request["results"];
    for(ssize_t index = 0; index < resultArray.arraySize() ; index++) {
        pbnjson::JValue deviceInfoObj = pbnjson::Object();
        for(ssize_t idx = 0; idx < resultArray[index]["storageDriveList"].arraySize() ; idx++) {
            std::string driveName =  resultArray[index]["storageDriveList"][idx]["driveName"].asString();
            std::string mountName =  resultArray[index]["storageDriveList"][idx]["mountName"].asString();
            if(driveName == m_requestedDrive){
                driveFound = true;
                if (object->isDriveBusy(mountName) == false ){
                    if(object->isGetSpaceInfoRequest) {
                        struct statfs fsInfo = {0};
                        if (statfs( mountName.c_str(), &fsInfo ) != 0) {
                            PDM_LOG_ERROR("PdmFs:%s line: %d statfs failed for mountName:%s ", __FUNCTION__, __LINE__, mountName.c_str());
                            json.put("returnValue", false);
                            json.put("errorText", "statfs failed");
                        } else {
                            int64_t driveSize = ( fsInfo.f_blocks * (fsInfo.f_bsize / 1024) );
                            int64_t freeSize = ( fsInfo.f_bavail * (fsInfo.f_bsize / 1024) );
                            int64_t usedRate = 0;
                            int64_t usedSize = 0;
                            if (driveSize > freeSize) {
                                usedSize = driveSize - freeSize;
                            }
                            if (driveSize) {
                                usedRate = usedSize * 100 / driveSize;
                            }
                            pbnjson::JValue spaceInfo = pbnjson::Object();
                            spaceInfo.put("driveSize",driveSize);
                            spaceInfo.put("freeSize", freeSize);
                            spaceInfo.put("usedRate", usedRate);
                            spaceInfo.put("usedSize", usedSize);
                            json.put("spaceInfo", spaceInfo);
                            json.put("returnValue", true);
                        }
                    } else {
                        json.put("returnValue", true);
                        json.put("isWritable", true);
                        if (!object->isWritable(driveName)){
                            json.put("isWritable", false);
                        }
                    }
                } else {
                    json.put("returnValue", false);
                    json.put("errorText", "drive is busy");
                }
                break;
            }
        }
    }
    if(!driveFound) {
        if(false == object->isGetSpaceInfoRequest) {
            json.put("errorCode", 1027);
            json.put("returnValue", false);
            json.put("errorText", "Device error unknown");
        } else {
            json.put("errorCode", 1022);
            json.put("returnValue", false);
            json.put("errorText", "Drive not found in device");
        }
    }
    if(!LSMessageReply( sh, requestedDriveReplyMsg, json.stringify(NULL).c_str(), &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        LSMessageUnref(requestedDriveReplyMsg);
    }
    return true;
}
bool PdmLunaService::isDriveBusy(std::string mountName)
{
    PDM_LOG_INFO("PdmLunaService:",0,"%s line: %d Mount Name: %s", __FUNCTION__,__LINE__,mountName.c_str());
    std::string sysCommand("lsof +D ");
    sysCommand.append(mountName);
    std::string result = PdmUtils::execShellCmd(sysCommand);
    if(!result.empty()){
        PDM_LOG_ERROR("PdmLunaService:%s line: %d error on isDriveBusy", __FUNCTION__, __LINE__);
        return true;
    }
     return false;
}

bool PdmLunaService::isWritable(std::string driveName)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    FILE *pfile;
    bool isWritable = false;
    std::string devName = "/dev/" + driveName;
    if ((pfile = setmntent("/etc/mtab", "r")) != NULL) {
        struct mntent *pmnt;
        while ((pmnt = getmntent(pfile)) != NULL)
        {
            if(strcmp(devName.c_str(), pmnt->mnt_fsname) == 0) {
                if (hasmntopt(pmnt, "ro") == NULL)
                {
                    isWritable = true;
                    break;
                }
            }
        }
    }
    endmntent (pfile);
    return isWritable;
}
#endif

bool PdmLunaService::cbUmountAllDrive(LSHandle *sh, LSMessage *message)
{
    PDM_LOG_DEBUG("PdmLunaService::cbumountAllDrive() ");
    UmountAllCommand *umountAllCmd = new UmountAllCommand;
    if(!umountAllCmd)
        return true;

    LSMessageRef(message);
    PdmCommand *cmdUmountAllDrive = new PdmCommand(reinterpret_cast<CommandType*>(umountAllCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );
    mCommandManager->sendCommand(cmdUmountAllDrive);

    return true;
}

bool PdmLunaService::notifySubscribers(int eventDeviceType, const int &eventID, std::string hubPortPath)
{
    PDM_LOG_DEBUG("PdmLunaService::notifySubscribers - Device Type: %s",DeviceEventTable[eventDeviceType]);

    bool bRetVal = false;
    LSError error;
    LSErrorInit(&error);
    pbnjson::JValue payload ;
#ifdef WEBOS_SESSION
    if((2 == eventID) && (eventDeviceType == STORAGE_DEVICE || eventDeviceType == NON_STORAGE_DEVICE)) {
        if(!hubPortPath.empty()) {
                if(!queryDevice(hubPortPath)) {
                    PDM_LOG_ERROR("PdmLunaService:%s line: %d Error in db", __FUNCTION__, __LINE__);
                }
        } else {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d hubPortPath is null ", __FUNCTION__, __LINE__);
        }
    }
#endif

    if(eventDeviceType == STORAGE_DEVICE)
        payload = createJsonGetAttachedStorageDeviceList(nullptr);
    else if(eventDeviceType != STORAGE_DEVICE && eventDeviceType != ALL_DEVICE)
        payload = createJsonGetAttachedNonStorageDeviceList(nullptr);

    if(eventDeviceType != ALL_DEVICE){
        // subscription reply
        bRetVal = LSSubscriptionReply(mServiceHandle, DeviceEventTable[eventDeviceType], payload.stringify(NULL).c_str(), &error);
        LSERROR_CHECK_AND_PRINT(bRetVal, error);
    }
    // Always notify who have subscribed for all device changes
    payload = createJsonGetAttachedDeviceStatus(nullptr);
    bRetVal = LSSubscriptionReply(mServiceHandle, DeviceEventTable[ALL_DEVICE], payload.stringify(NULL).c_str(), &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);

#ifdef WEBOS_SESSION
    if((eventDeviceType == NON_STORAGE_DEVICE) || (eventDeviceType == STORAGE_DEVICE)) {
         payload = createJsonGetAttachedAllDeviceList(nullptr);
         bRetVal = LSSubscriptionReply(mServiceHandle,PDM_EVENT_ALL_ATTACHED_DEVICE_LIST, payload.stringify(NULL).c_str(), &error);
         LSERROR_CHECK_AND_PRINT(bRetVal, error);
    }
#endif
    return true;
}

bool PdmLunaService::subscriptionAdd(LSHandle *a_sh, const char *a_key, LSMessage *a_message)
{
    LSError lsError;
    LSErrorInit(&lsError);

    if ( !LSSubscriptionAdd(a_sh, a_key, a_message, &lsError) ) {
        LSErrorFree(&lsError);
        return false;
    }
    return true;
}


bool PdmLunaService::cbmountandFullFsck(LSHandle *sh, LSMessage *message)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));
    const char* payload = LSMessageGetPayload(message);

    if (payload) {
        pbnjson::JValue list = pbnjson::JDomParser::fromString(payload);

        MountFsckCommand *mountFsckCmd = new MountFsckCommand;
        if(!mountFsckCmd)
            return true;
        mountFsckCmd->mountName = list["mountName"].asString();
        mountFsckCmd->needFsck = list["needFsck"].asBool();
        LSMessageRef(message);
        PdmCommand *cmdFsck = new PdmCommand(reinterpret_cast<CommandType*>(mountFsckCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );
        mCommandManager->sendCommand(cmdFsck);
    }
    return true;
}

#ifdef WEBOS_SESSION

void PdmLunaService::notifyResumeDone() {
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d ", __FUNCTION__, __LINE__);
    sleep(2);
    deleteDeviceFromDb("RSE-R");
    deleteDeviceFromDb("RSE-L");
    deleteDeviceFromDb("AVN");
}
bool PdmLunaService::cbgetAttachedAllDeviceList(LSHandle *sh, LSMessage *message) {
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));
    bool bRetVal;
    LSError error;
    LSErrorInit(&error);
    pbnjson::JValue obj= createJsonGetAttachedAllDeviceList(message);
    if (LSMessageIsSubscription(message))
    {
        subscriptionAdd(sh,PDM_EVENT_ALL_ATTACHED_DEVICE_LIST, message);
    }
    bRetVal  =  LSMessageReply (sh,  message,  obj.stringify(NULL).c_str() ,  &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
    return true;
}

pbnjson::JValue PdmLunaService::createJsonGetAttachedAllDeviceList(LSMessage *message) {
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));
    pbnjson::JValue deviceInfoArray = pbnjson::Array();
    pbnjson::JValue storageDevicePayload = createJsonGetAttachedStorageDeviceList(message);
    deviceInfoArray.put(0, storageDevicePayload);
    pbnjson::JValue nonStorageDevicePayload = createJsonGetAttachedNonStorageDeviceList(message);
    deviceInfoArray.put(1, nonStorageDevicePayload);
    pbnjson::JValue json = pbnjson::Object();
    json.put("returnValue", "true");
    json.put("deviceListInfo", deviceInfoArray);
    return json;
}

bool PdmLunaService::cbSetDeviceForSession(LSHandle *sh, LSMessage *message)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));
    bool bRetVal = false, success = false;
    LSError error;
    LSErrorInit(&error);
    bool subscribed = false;
    std::string deviceSetId = "";
    std::string lastNonStorageDeviceSetId = "";
    std::string lastStorageDeviceSetId = "";
    pbnjson::JValue response = pbnjson::Object();
    std::string curSessionId = LSMessageGetSessionId(message);
    std::string prevSessionId = "";
    queryForSession();
    const char* payload = LSMessageGetPayload(message);
    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload empty", __FUNCTION__, __LINE__);
        return true;
    }
    pbnjson::JValue list = pbnjson::JDomParser::fromString(payload);

    PDM_LOG_DEBUG("PdmLunaService:%s line: %d list:%s", __FUNCTION__, __LINE__, list.stringify().c_str());

    pbnjson::JValue storageList = pbnjson::Object();
    if (list["deviceListInfo"][0].hasKey("storageDeviceList"))
        storageList = list["deviceListInfo"][0];
    else if(list["deviceListInfo"][1].hasKey("storageDeviceList"))
        storageList = list["deviceListInfo"][1];

    PDM_LOG_DEBUG("PdmLunaService:%s line: %d storageList:%s", __FUNCTION__, __LINE__, storageList.stringify().c_str());
    pbnjson::JValue avnStorageDeviceArray = pbnjson::Array();
    pbnjson::JValue rserStorageDeviceArray = pbnjson::Array();
    pbnjson::JValue rselStorageDeviceArray = pbnjson::Array();
    pbnjson::JValue hostStorageDeviceArray = pbnjson::Array();

    pbnjson::JValue avnAllDeviceArray = pbnjson::Array();
    pbnjson::JValue rserAllDeviceArray = pbnjson::Array();
    pbnjson::JValue rselAllDeviceArray = pbnjson::Array();

    int avnStorageDeviceNo = 0; int rselStorageDeviceNo = 0; int rserStorageDeviceNo = 0; int hostStorageDeviceNo = 0;
    int avnAllDeviceNo = 0; int rselAllDeviceNo = 0; int rserAllDeviceNo = 0;
    bool sDeviceConnectedAvn = false; bool sDeviceConnectedRseL = false; bool sDeviceConnectedRseR = false;
    bool allDeviceConnectedAvn = false; bool allDeviceConnectedRseL = false; bool allDeviceConnectedRseR = false;
    for (auto& device : storageList["storageDeviceList"].items())
    {
        PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
        std::string deviceSetId = device["deviceSetId"].asString();
        std::string hubPortPath = device["hubPortPath"].asString();
        std::string deviceType = device["deviceType"].asString();
        auto devicePair = std::find_if(std::begin(m_portDisplayMap), std::end(m_portDisplayMap), [&](const std::pair<std::string, std::string> &pair)
        {
            return (pair.first == hubPortPath);
        });

        if (devicePair != std::end(m_portDisplayMap))
        {
            lastStorageDeviceSetId = devicePair->second;
        }
        prevSessionId = findPreviousSessionId(hubPortPath);
        PDM_LOG_DEBUG("PdmLunaService:%s line: %d prevSessionId: %s, curSessionId: %s", __FUNCTION__, __LINE__, prevSessionId.c_str(), curSessionId.c_str());
        PDM_LOG_DEBUG("PdmLunaService:%s line: %d lastStorageDeviceSetId: %s, currentDeviceSetId: %s", __FUNCTION__, __LINE__, lastStorageDeviceSetId.c_str(), deviceSetId.c_str());
        if ((!prevSessionId.empty() && prevSessionId != curSessionId))
        {
            deletePreviousMountName(hubPortPath);
            deletePreviousPayload(hubPortPath);
            if((!lastStorageDeviceSetId.empty()) && (lastStorageDeviceSetId != deviceSetId)) {
                displayDisconnectedToast("Storage", lastStorageDeviceSetId);
            }
        }
        m_portDisplayMap[hubPortPath] = deviceSetId;
        m_hubPortPathSessionIdMap[hubPortPath] = curSessionId;
        if (!deviceSetId.empty() && (deviceSetId != "Select")) {
            success = storeDeviceInfo(device);
        }
        PDM_LOG_DEBUG("PdmLunaService:%s line: %d success:%d", __FUNCTION__, __LINE__, success);
        std::string storageDeviceType = device["deviceType"].asString();
        PDM_LOG_DEBUG("PdmLunaService:%s line: %d storageDeviceType: %s deviceSetId: %s", __FUNCTION__, __LINE__, storageDeviceType.c_str(), deviceSetId.c_str());

        if(deviceSetId == "RSE-L") {
            sDeviceConnectedRseL = true;allDeviceConnectedRseL = true;
            rselStorageDeviceArray.put(rselStorageDeviceNo, device);
            hostStorageDeviceArray.put(hostStorageDeviceNo,device);
            rselAllDeviceArray.put(rselAllDeviceNo, device);
            rselStorageDeviceNo++; hostStorageDeviceNo++; rselAllDeviceNo++;
        }

        if(deviceSetId == "RSE-R") {
            sDeviceConnectedRseR = true;allDeviceConnectedRseR=true;
            rserStorageDeviceArray.put(rserStorageDeviceNo, device);
            hostStorageDeviceArray.put(hostStorageDeviceNo,device);
            rserAllDeviceArray.put(rserAllDeviceNo, device);
            rserStorageDeviceNo++; hostStorageDeviceNo++;rserAllDeviceNo++;
        }

        if(deviceSetId == "AVN") {
            sDeviceConnectedAvn = true;allDeviceConnectedAvn = true;
            avnStorageDeviceArray.put(avnStorageDeviceNo, device);
            hostStorageDeviceArray.put(hostStorageDeviceNo,device);
            avnAllDeviceArray.put(avnAllDeviceNo, device);
            avnStorageDeviceNo++;hostStorageDeviceNo++;avnAllDeviceNo++;
        }

        if(!lastStorageDeviceSetId.empty()) {
            pbnjson::JValue deviceListArray = pbnjson::Array();
            pbnjson::JValue deviceListInfo = pbnjson::Object();
            pbnjson::JValue payload = pbnjson::Object();
            if(lastStorageDeviceSetId == "AVN") {
              payload.put("storageDeviceList", avnStorageDeviceArray);
            }else if (lastStorageDeviceSetId == "RSE-R") {
              payload.put("storageDeviceList", rserStorageDeviceArray);
            }else if(lastStorageDeviceSetId == "RSE-L") {
              payload.put("storageDeviceList", rselStorageDeviceArray);
            }
            deviceListArray.put(0, payload);
            deviceListInfo.put("deviceListInfo", deviceListArray);
            notifyToDisplay(deviceListInfo, lastStorageDeviceSetId,"USB_STORAGE" );
        }

        if (storageDeviceType == "USB_STORAGE") {
            for (auto& drive : device["storageDriveList"].items()) {
                std::string driveName = drive["driveName"].asString();
                std::string mountName = drive["mountName"].asString();
                std::string fsType = drive["fsType"].asString();
                if (fsType == "tntfs" || fsType == "ntfs" || fsType == "vfat" || fsType == "tfat") {
                    success = mountDeviceToSession( mountName, driveName, deviceSetId, fsType);
                    updateIsMount(device,driveName);
                }
                response.put("returnValue", success);
                PDM_LOG_DEBUG("PdmLunaService:%s line: %d success:%d fsType:%s", __FUNCTION__, __LINE__, success,fsType.c_str());
            }
            updateErrorReason(device);
        }
        displayConnectedToast("Storage",deviceSetId);
    }

    pbnjson::JValue deviceListArray = pbnjson::Array();
    pbnjson::JValue deviceListInfo = pbnjson::Object();

    if(sDeviceConnectedAvn) {
        pbnjson::JValue avnPayload = pbnjson::Object();
        avnPayload.put("storageDeviceList", avnStorageDeviceArray);
        deviceListArray.put(0, avnPayload);
        deviceListInfo.put("deviceListInfo", deviceListArray);
        std::string deviceSetId = "AVN";
        notifyToDisplay(deviceListInfo, deviceSetId,"USB_STORAGE" );
    }

    if(sDeviceConnectedRseR) {
        pbnjson::JValue rseRpayload = pbnjson::Object();
        rseRpayload.put("storageDeviceList", rserStorageDeviceArray);
        deviceListArray.put(0, rseRpayload);
        deviceListInfo.put("deviceListInfo", deviceListArray);
        std::string deviceSetId = "RSE-R";
        notifyToDisplay(deviceListInfo, deviceSetId,"USB_STORAGE" );
    }

    if(sDeviceConnectedRseL){
        pbnjson::JValue rseLPayload = pbnjson::Object();
        rseLPayload.put("storageDeviceList",rselStorageDeviceArray );
        deviceListArray.put(0, rseLPayload);
        deviceListInfo.put("deviceListInfo", deviceListArray);
        std::string deviceSetId = "RSE-L";
        notifyToDisplay(deviceListInfo, deviceSetId,"USB_STORAGE" );
    }

    pbnjson::JValue hostStoragePayload = pbnjson::Object();
    hostStoragePayload.put("storageDeviceList",hostStorageDeviceArray );
    deviceListArray.put(0, hostStoragePayload);
    deviceListInfo.put("deviceListInfo", deviceListArray);
    std::string deviceSetIdHost = "HOST";
    notifyToDisplay(deviceListInfo, deviceSetIdHost,"USB_STORAGE" );


    pbnjson::JValue nonStorageList = pbnjson::Object();
    if (list["deviceListInfo"][0].hasKey("nonStorageDeviceList"))
        nonStorageList = list["deviceListInfo"][0];
    else if(list["deviceListInfo"][1].hasKey("nonStorageDeviceList"))
        nonStorageList = list["deviceListInfo"][1];

    PDM_LOG_DEBUG("PdmLunaService:%s line: %d nonStorageList:%s", __FUNCTION__, __LINE__, nonStorageList.stringify().c_str());
    pbnjson::JValue avnDeviceArray = pbnjson::Array();
    pbnjson::JValue rserDeviceArray = pbnjson::Array();
    pbnjson::JValue rselDeviceArray = pbnjson::Array();
    pbnjson::JValue hostDeviceArray = pbnjson::Array();

    int avnDeviceNo = 0; int rselDeviceNo = 0; int rserDeviceNo = 0; int hostDeviceNo = 0;
    bool nsDeviceConnectedAvn = false; bool nsDeviceConnectedRseL = false; bool nsDeviceConnectedRseR = false;

    for (auto& device : nonStorageList["nonStorageDeviceList"].items())
    {
        PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
        std::string deviceSetId = device["deviceSetId"].asString();
        std::string hubPortPath = device["hubPortPath"].asString();
        std::string nonStorageDeviceType = device["deviceType"].asString();
        auto devicePair = std::find_if(std::begin(m_portDisplayMap), std::end(m_portDisplayMap), [&](const std::pair<std::string, std::string> &pair)
        {
            return (pair.first == hubPortPath);
        });

        if (devicePair != std::end(m_portDisplayMap))
        {
            lastNonStorageDeviceSetId = devicePair->second;
        }

        PDM_LOG_DEBUG("PdmLunaService:%s line: %d lastNonStorageDeviceSetId: %s, currentDeviceSetId: %s", __FUNCTION__, __LINE__, lastNonStorageDeviceSetId.c_str(), deviceSetId.c_str());
        if ((!lastNonStorageDeviceSetId.empty()) && (lastNonStorageDeviceSetId != deviceSetId))
        {
            deletePreviousPayload(hubPortPath);
            displayDisconnectedToast(nonStorageDeviceType,lastNonStorageDeviceSetId);
        }
        m_portDisplayMap[hubPortPath] = deviceSetId;
        if (!deviceSetId.empty() && (deviceSetId != "Select")) {
            success = storeDeviceInfo(device);
        }
        PDM_LOG_DEBUG("PdmLunaService:%s line: %d success:%d ", __FUNCTION__, __LINE__, success);
        response.put("returnValue", success);
        deviceSetId = device["deviceSetId"].asString();
        std::string devPath = device["devPath"].asString();

        if(deviceSetId == "RSE-L") {
            nsDeviceConnectedRseL = true;allDeviceConnectedRseL=true;
            rselDeviceArray.put(rselDeviceNo, device);
            hostDeviceArray.put(hostDeviceNo,device);
            rselAllDeviceArray.put(rselAllDeviceNo, device);
            rselDeviceNo++; hostDeviceNo++;rselAllDeviceNo++;
            PdmUtils::do_chown(devPath.c_str(), rselUserId.c_str(), rselUserId.c_str());
            if(0 != chmod(devPath.c_str(), NON_STORAGE_FILE_MODE)) {
                PDM_LOG_ERROR("PdmLunaService:%s line:%d chmod error: %d stderror: %s", __FUNCTION__, __LINE__, errno, strerror(errno));
            }
            PDM_LOG_DEBUG("PdmLunaService:%s line: %d devPath: %s", __FUNCTION__, __LINE__, devPath.c_str());
        }
        if(deviceSetId == "RSE-R") {
            nsDeviceConnectedRseR = true;allDeviceConnectedRseR = true;
            rserDeviceArray.put(rserDeviceNo, device);
            hostDeviceArray.put(hostDeviceNo,device);
            rserAllDeviceArray.put(rserAllDeviceNo, device);
            rserDeviceNo++; hostDeviceNo++;rserAllDeviceNo++;
            PdmUtils::do_chown(devPath.c_str(), rserUserId.c_str(), rserUserId.c_str());
            if(0 != chmod(devPath.c_str(), NON_STORAGE_FILE_MODE)) {
                PDM_LOG_ERROR("PdmLunaService:%s line:%d chmod error: %d stderror: %s", __FUNCTION__, __LINE__, errno, strerror(errno));
            }
            PDM_LOG_DEBUG("PdmLunaService:%s line: %d devPath: %s", __FUNCTION__, __LINE__, devPath.c_str());
        }

        if(deviceSetId == "AVN") {
            nsDeviceConnectedAvn = true; allDeviceConnectedAvn = true;
            avnDeviceArray.put(avnDeviceNo, device);
            hostDeviceArray.put(hostDeviceNo,device);
            avnAllDeviceArray.put(avnAllDeviceNo, device);
            avnDeviceNo++;hostDeviceNo++;avnAllDeviceNo++;
            PdmUtils::do_chown(devPath.c_str(), avnUserId.c_str(), avnUserId.c_str());
            if(0 != chmod(devPath.c_str(), NON_STORAGE_FILE_MODE)){
                PDM_LOG_ERROR("PdmLunaService:%s line:%d chmod error: %d stderror: %s", __FUNCTION__, __LINE__, errno, strerror(errno));
            }
            PDM_LOG_DEBUG("PdmLunaService:%s line: %d devPath: %s", __FUNCTION__, __LINE__, devPath.c_str());
        }
        if(!lastNonStorageDeviceSetId.empty()) {
            pbnjson::JValue deviceListArray = pbnjson::Array();
            pbnjson::JValue deviceListInfo = pbnjson::Object();
            pbnjson::JValue payload = pbnjson::Object();
            if(lastNonStorageDeviceSetId == "AVN") {
              payload.put("nonStorageDeviceList", avnDeviceArray);
            }else if (lastNonStorageDeviceSetId == "RSE-R") {
              payload.put("nonStorageDeviceList", rserDeviceArray);
            }else if(lastNonStorageDeviceSetId == "RSE-L") {
              payload.put("nonStorageDeviceList", rselDeviceArray);
            }
            deviceListArray.put(0, payload);
            deviceListInfo.put("deviceListInfo", deviceListArray);
            notifyToDisplay(deviceListInfo, lastNonStorageDeviceSetId,"USB_NONSTORAGE" );
        }

        displayConnectedToast(nonStorageDeviceType, deviceSetId);
    }

    if(nsDeviceConnectedAvn) {
        pbnjson::JValue avnPayload = pbnjson::Object();
        avnPayload.put("nonStorageDeviceList", avnDeviceArray);
        deviceListArray.put(0, avnPayload);
        deviceListInfo.put("deviceListInfo", deviceListArray);
        std::string deviceSetId = "AVN";
        notifyToDisplay(deviceListInfo, deviceSetId,"USB_NONSTORAGE" );
    }

    if(nsDeviceConnectedRseR) {
        pbnjson::JValue rseRpayload = pbnjson::Object();
        rseRpayload.put("nonStorageDeviceList", rserDeviceArray);
        deviceListArray.put(0, rseRpayload);
        deviceListInfo.put("deviceListInfo", deviceListArray);
        std::string deviceSetId = "RSE-R";
        notifyToDisplay(deviceListInfo, deviceSetId,"USB_NONSTORAGE" );
    }

    if(nsDeviceConnectedRseL){
        pbnjson::JValue rseLPayload = pbnjson::Object();
        rseLPayload.put("nonStorageDeviceList",rselDeviceArray );
        deviceListArray.put(0, rseLPayload);
        deviceListInfo.put("deviceListInfo", deviceListArray);
        std::string deviceSetId = "RSE-L";
        notifyToDisplay(deviceListInfo, deviceSetId,"USB_NONSTORAGE" );
    }

    pbnjson::JValue hostPayload = pbnjson::Object();
    hostPayload.put("nonStorageDeviceList",hostDeviceArray );
    deviceListArray.put(0, hostPayload);
    deviceListInfo.put("deviceListInfo", deviceListArray);
    std::string deviceSetIdHostNonStorage = "HOST";
    notifyToDisplay(deviceListInfo, deviceSetIdHostNonStorage,"USB_NONSTORAGE" );

    list.put("returnValue", true);
    bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_ALL_ATTACHED_DEVICE_LIST, list.stringify(NULL).c_str(), &error);

    if(!lastNonStorageDeviceSetId.empty()) {
        PDM_LOG_DEBUG("PdmLunaService::%s line:%d lastNonStorageDeviceSetId:%s" ,__FUNCTION__, __LINE__,lastNonStorageDeviceSetId.c_str());
        pbnjson::JValue devicePayload = pbnjson::Array();
        if(lastNonStorageDeviceSetId == "AVN") {
            pbnjson::JValue storageDeviceList = getStorageDevicePayload(avnAllDeviceArray);
            devicePayload.put(0, storageDeviceList);
            pbnjson::JValue nonStorageDeviceList = getNonStorageDevicePayload(avnAllDeviceArray);
            devicePayload.put(1, nonStorageDeviceList);
        }

        if(lastNonStorageDeviceSetId == "RSE-R") {
            pbnjson::JValue storageDeviceList = getStorageDevicePayload(rserAllDeviceArray);
            devicePayload.put(0, storageDeviceList);
            pbnjson::JValue nonStorageDeviceList = getNonStorageDevicePayload(rserAllDeviceArray);
            devicePayload.put(1, nonStorageDeviceList);
        }

        if(lastNonStorageDeviceSetId == "RSE-L") {
            pbnjson::JValue storageDeviceList = getStorageDevicePayload(rselAllDeviceArray);
            devicePayload.put(0, storageDeviceList);
            pbnjson::JValue nonStorageDeviceList = getNonStorageDevicePayload(rselAllDeviceArray);
            devicePayload.put(1, nonStorageDeviceList);
        }

        pbnjson::JValue json = pbnjson::Object();
        json.put("returnValue", true);
        json.put("deviceListInfo", devicePayload);
        notifyAllDeviceToDisplay(lastNonStorageDeviceSetId,json);
    }

    if(!lastStorageDeviceSetId.empty()) {
        PDM_LOG_DEBUG("PdmLunaService::%s line:%d lastStorageDeviceSetId: %s" ,__FUNCTION__, __LINE__, lastStorageDeviceSetId.c_str());
        pbnjson::JValue devicePayload = pbnjson::Array();
        if(lastStorageDeviceSetId == "AVN") {
            pbnjson::JValue storageDeviceList = getStorageDevicePayload(avnAllDeviceArray);
            devicePayload.put(0, storageDeviceList);
            pbnjson::JValue nonStorageDeviceList = getNonStorageDevicePayload(avnAllDeviceArray);
            devicePayload.put(1, nonStorageDeviceList);
        }

        if(lastStorageDeviceSetId == "RSE-R") {
            pbnjson::JValue storageDeviceList = getStorageDevicePayload(rserAllDeviceArray);
            devicePayload.put(0, storageDeviceList);
            pbnjson::JValue nonStorageDeviceList = getNonStorageDevicePayload(rserAllDeviceArray);
            devicePayload.put(1, nonStorageDeviceList);
        }
        if(lastStorageDeviceSetId == "RSE-L") {
            pbnjson::JValue storageDeviceList = getStorageDevicePayload(rselAllDeviceArray);
            devicePayload.put(0, storageDeviceList);
            pbnjson::JValue nonStorageDeviceList = getNonStorageDevicePayload(rselAllDeviceArray);
            devicePayload.put(1, nonStorageDeviceList);
        }

        pbnjson::JValue json = pbnjson::Object();
        json.put("returnValue", true);
        json.put("deviceListInfo", devicePayload);
        notifyAllDeviceToDisplay(lastStorageDeviceSetId,json);
    }

    if(allDeviceConnectedAvn) {
        PDM_LOG_DEBUG("PdmLunaService::%s line:%d allDeviceConnectedAvn" ,__FUNCTION__, __LINE__);
        pbnjson::JValue avnAllPayload = pbnjson::Array();
        pbnjson::JValue storageDeviceList = getStorageDevicePayload(avnAllDeviceArray);
        avnAllPayload.put(0, storageDeviceList);
        pbnjson::JValue nonStorageDeviceList = getNonStorageDevicePayload(avnAllDeviceArray);
        avnAllPayload.put(1, nonStorageDeviceList);
        pbnjson::JValue json = pbnjson::Object();
        json.put("returnValue", true);
        json.put("deviceListInfo", avnAllPayload);
        std::string deviceSetId = "AVN";
        notifyAllDeviceToDisplay(deviceSetId,json);
    }
    if(allDeviceConnectedRseL) {
        PDM_LOG_DEBUG("PdmLunaService::%s line:%d allDeviceConnectedRseL" ,__FUNCTION__, __LINE__);
        pbnjson::JValue rselAllPayload = pbnjson::Array();
        pbnjson::JValue storageDeviceList = getStorageDevicePayload(rselAllDeviceArray);
        rselAllPayload.put(0, storageDeviceList);
        pbnjson::JValue nonStorageDeviceList = getNonStorageDevicePayload(rselAllDeviceArray);
        rselAllPayload.put(1, nonStorageDeviceList);
        pbnjson::JValue json = pbnjson::Object();
        json.put("returnValue", true);
        json.put("deviceListInfo", rselAllPayload);
        std::string deviceSetId = "RSE-L";
        notifyAllDeviceToDisplay(deviceSetId,json);
    }

    if(allDeviceConnectedRseR) {
        PDM_LOG_DEBUG("PdmLunaService::%s line:%d allDeviceConnectedRseR" ,__FUNCTION__, __LINE__);
        pbnjson::JValue rserAllPayload = pbnjson::Array();
        pbnjson::JValue storageDeviceList = getStorageDevicePayload(rserAllDeviceArray);
        rserAllPayload.put(0, storageDeviceList);
        pbnjson::JValue nonStorageDeviceList = getNonStorageDevicePayload(rserAllDeviceArray);
        rserAllPayload.put(1, nonStorageDeviceList);
        pbnjson::JValue json = pbnjson::Object();
        json.put("returnValue", true);
        json.put("deviceListInfo", rserAllPayload);
        std::string deviceSetId = "RSE-R";
        notifyAllDeviceToDisplay(deviceSetId, json);
    }

    bRetVal  =  LSMessageReply (sh, message, response.stringify(NULL).c_str(), &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
    return true;
}

std::string PdmLunaService::findPreviousSessionId(std::string hubPortPath)
{
    std::string previousSessionId;
    auto devicePair = std::find_if(std::begin(m_hubPortPathSessionIdMap), std::end(m_hubPortPathSessionIdMap), [&](const std::pair<std::string, std::string> &pair)
    {
        return (pair.first == hubPortPath);
    });

    if (devicePair != std::end(m_hubPortPathSessionIdMap))
    {
        previousSessionId = devicePair->second;
    }
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d previousSessionId: %s", __FUNCTION__, __LINE__, previousSessionId.c_str());
    return previousSessionId;
}

bool PdmLunaService::queryForSession()
{
    bool bRetVal = false;
    LSError error;
    LSErrorInit(&error);

    pbnjson::JValue payload = pbnjson::Object();
    payload.put("subscribe", true);

    LS::Payload find_payload(payload);
    LS::Call call = mServiceCPPHandle->callOneReply("luna://com.webos.service.sessionmanager/getSessionList", find_payload.getJson(), NULL, this, NULL);
    LS::Message message = call.get();

    LS::PayloadRef response_payload = message.accessPayload();
    pbnjson::JValue request = response_payload.getJValue();

    std::string deviceSetId0, deviceSetId1, deviceSetId2 = "";
    std::string sessionId0, sessionId1, sessionId2 = "";
    std::map<std::string, std::string>::iterator it;

    deviceSetId0 = request["sessionList"][0]["deviceSetInfo"]["deviceSetId"].asString();
    sessionId0 = request["sessionList"][0]["sessionId"].asString();
    PDM_LOG_DEBUG("PdmLunaService: %s line: %d DeviceSetId0: %s, SessionId0: %s", __FUNCTION__, __LINE__, deviceSetId0.c_str(), sessionId0.c_str());
    it = m_sessionMap.find(deviceSetId0);
    if (it != m_sessionMap.end()) {
        it->second = sessionId0;
        PDM_LOG_DEBUG("PdmLunaService: %s line: %d Updated new SessionId: %s", __FUNCTION__, __LINE__,sessionId0.c_str());
    } else {
        m_sessionMap.insert(std::pair<std::string, std::string>(deviceSetId0, sessionId0));
    }
    deviceSetId1 = request["sessionList"][1]["deviceSetInfo"]["deviceSetId"].asString();
    sessionId1 = request["sessionList"][1]["sessionId"].asString();
    it = m_sessionMap.find(deviceSetId1);
    PDM_LOG_DEBUG("PdmLunaService: %s line: %d DeviceSetId1: %s, SessionId1: %s", __FUNCTION__, __LINE__, deviceSetId1.c_str(), sessionId1.c_str());
    if (it != m_sessionMap.end()) {
        it->second = sessionId1;
        PDM_LOG_DEBUG("PdmLunaService: %s line: %d Updated new SessionId: %s", __FUNCTION__, __LINE__,sessionId1.c_str());
    } else {
        m_sessionMap.insert(std::pair<std::string, std::string>(deviceSetId1, sessionId1));
    }
    if(!request["sessionList"][2].isNull())
    {
        deviceSetId2 = request["sessionList"][2]["deviceSetInfo"]["deviceSetId"].asString();
        sessionId2 = request["sessionList"][2]["sessionId"].asString();
        PDM_LOG_DEBUG("PdmLunaService: %s line: %d DeviceSetId2: %s, SessionId2: %s", __FUNCTION__, __LINE__, deviceSetId2.c_str(), sessionId2.c_str());
        it = m_sessionMap.find(deviceSetId2);
        if (it != m_sessionMap.end()) {
            it->second = sessionId2;
            PDM_LOG_DEBUG("PdmLunaService: %s line: %d Updated new SessionId: %s", __FUNCTION__, __LINE__,sessionId2.c_str());
        } else {
            m_sessionMap.insert(std::pair<std::string, std::string>(deviceSetId2, sessionId2));
        }
    }

    if (deviceSetId0 == "AVN")
            avnUserId = request["sessionList"][0]["userInfo"]["userId"].asString();
    else if (deviceSetId1 == "AVN")
            avnUserId = request["sessionList"][1]["userInfo"]["userId"].asString();
    else if ((!deviceSetId2.empty()) && (deviceSetId2 == "AVN"))
            avnUserId = request["sessionList"][2]["userInfo"]["userId"].asString();

    if (deviceSetId0 == "RSE-L")
            rselUserId = request["sessionList"][0]["userInfo"]["userId"].asString();
    else if (deviceSetId1 == "RSE-L")
            rselUserId = request["sessionList"][1]["userInfo"]["userId"].asString();
    else if ((!deviceSetId2.empty()) && (deviceSetId2 == "RSE-L"))
            rselUserId = request["sessionList"][2]["userInfo"]["userId"].asString();

    if (deviceSetId0 == "RSE-R")
            rserUserId = request["sessionList"][0]["userInfo"]["userId"].asString();
    else if (deviceSetId1 == "RSE-R")
            rserUserId = request["sessionList"][1]["userInfo"]["userId"].asString();
    else if ((!deviceSetId2.empty()) && (deviceSetId2 == "RSE-R"))
            rserUserId = request["sessionList"][2]["userInfo"]["userId"].asString();

    return true;
}

bool PdmLunaService::updateIsMount(pbnjson::JValue list, std::string driveName)
{
    LSError lserror;
    LSErrorInit(&lserror);

    //Add kind to the object
    list.put("_kind", DB8_KIND);
    pbnjson::JValue mergeput_query = pbnjson::Object();
    pbnjson::JValue query;
    pbnjson::JValue props = pbnjson::Object();

    std::string hubPortPath = list["hubPortPath"].asString();
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d hubPortPath: %s", __FUNCTION__, __LINE__, hubPortPath.c_str());
    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                              {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};

    struct statfs fsInfo = {0};
    int32_t driveSize;
    for(ssize_t idx = 0; idx < list["storageDriveList"].arraySize() ; idx++) {
        std::string partitionName = list["storageDriveList"][idx]["driveName"].asString();
        std::string mountName = list["storageDriveList"][idx]["mountName"].asString();
        if(partitionName == driveName) {
            list["storageDriveList"][idx].put("isMounted", true);
            if (statfs( mountName.c_str(), &fsInfo ) != 0) {
                PDM_LOG_ERROR("PdmFs:%s line: %d statfs failed for mountName:%s ", __FUNCTION__, __LINE__, mountName.c_str());
                list["storageDriveList"][idx].put("driveSize",0);
            } else {
                driveSize = ( fsInfo.f_blocks * (fsInfo.f_bsize / 1024) );
                list["storageDriveList"][idx].put("driveSize",driveSize);
            }
            break;
        }
    }
    mergeput_query.put("query", query);
    mergeput_query.put("props", list);

    PDM_LOG_DEBUG("PdmLunaService::%s line:%d MergePutPayload: %s", __FUNCTION__, __LINE__, mergeput_query.stringify().c_str());
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/mergePut",
                       mergeput_query.stringify().c_str(), cbDb8Response, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("Store device info to History table call failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        return false;
    }
    return true;
}

bool PdmLunaService::updateErrorReason(pbnjson::JValue list)
{
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d ", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);

    //Add kind to the object
    list.put("_kind", DB8_KIND);
    pbnjson::JValue mergeput_query = pbnjson::Object();
    pbnjson::JValue query;
    pbnjson::JValue props = pbnjson::Object();

    std::string hubPortPath = list["hubPortPath"].asString();
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d hubPortPath: %s", __FUNCTION__, __LINE__, hubPortPath.c_str());
    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                               {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};

    if (list["deviceType"].asString() == "USB_STORAGE")
    {
        list.put("errorReason","nothing");
    }
    mergeput_query.put("query", query);
    mergeput_query.put("props", list);

    PDM_LOG_DEBUG("PdmLunaService::%s line:%d MergePutPayload: %s", __FUNCTION__, __LINE__, mergeput_query.stringify().c_str());
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/mergePut",
                       mergeput_query.stringify().c_str(), cbDb8Response, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("Store device info to History table call failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        return false;
    }
    return true;
}

bool PdmLunaService::storeDeviceInfo(pbnjson::JValue list)
{
    LSError lserror;
    LSErrorInit(&lserror);

    //Add kind to the object
    list.put("_kind", DB8_KIND);
    pbnjson::JValue mergeput_query = pbnjson::Object();
    pbnjson::JValue query;
    pbnjson::JValue props = pbnjson::Object();

    std::string hubPortPath = list["hubPortPath"].asString();
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d hubPortPath: %s", __FUNCTION__, __LINE__, hubPortPath.c_str());
    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                               {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};

    if (list["deviceType"].asString() == "USB_STORAGE")
    {
        std::string rootPath;
        for(ssize_t idx = 0; idx < list["storageDriveList"].arraySize() ; idx++) {
            std::string fsType = list["storageDriveList"][idx]["fsType"].asString();
            if (fsType == "tntfs" || fsType == "ntfs" || fsType == "vfat" || fsType == "tfat")
            {
                std::string driveName = list["storageDriveList"][idx]["driveName"].asString();
                rootPath = "/tmp/usb/" + driveName.substr(0, 3);
                std::string mountName = rootPath + "/" + driveName;
                PdmUtils::createDir(mountName);
                list["storageDriveList"][idx].put("mountName", mountName);
            }
            else
            {
                list["storageDriveList"][idx].put("mountName", "UNSUPPORTED_FILESYSTEM");
            }
        }
        list.put("rootPath", rootPath);
    }
    mergeput_query.put("query", query);
    mergeput_query.put("props", list);

    LS::Payload merge_payload(mergeput_query);
    LS::Call call = LunaIPC::getInstance()->getLSCPPHandle()->callOneReply("luna://com.webos.service.db/mergePut", merge_payload.getJson(), NULL, this, NULL);
    LS::Message message = call.get();

    LS::PayloadRef response_payload = message.accessPayload();
    pbnjson::JValue request = response_payload.getJValue();

    if(request.isNull())
    {
        PDM_LOG_ERROR("PdmLunaService::%s line: %d not able to merge from Db ",  __FUNCTION__, __LINE__);
        return false;
    }

    if(!request["returnValue"].asBool())
    {
        PDM_LOG_ERROR("PdmLunaService::%s line: %d not able to merge from Db",  __FUNCTION__, __LINE__);
        return false;
    }
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d updated the db for hubPortPath: %s", __FUNCTION__, __LINE__, hubPortPath.c_str());
    return true;
}

bool PdmLunaService::cbDb8Response(LSHandle* lshandle, LSMessage *message, void *user_data)
{
    LSError lserror;
    LSErrorInit(&lserror);

    pbnjson::JValue request;

    const char* payload = LSMessageGetPayload(message);
    request = pbnjson::JDomParser::fromString(payload);

    if(request.isNull())
    {
        PDM_LOG_DEBUG("Db8 LS2 response is empty in %s", __PRETTY_FUNCTION__ );
        return false;
    }

    if(!request["returnValue"].asBool())
    {
        PDM_LOG_DEBUG("Call to Db8 to save/delete message failed in %s", __PRETTY_FUNCTION__ );
        return false;
    }

    PDM_LOG_DEBUG("[DB8Response] result:%s", LSMessageGetPayload(message));

    return true;
}

pbnjson::JValue PdmLunaService::getStorageDevicePayload(pbnjson::JValue resultArray) {

    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    pbnjson::JValue payload = pbnjson::Object();
    pbnjson::JValue storageDeviceList = pbnjson::Array();
    int storageDeviceCount = 0;
    for(ssize_t index = 0; index < resultArray.arraySize() ; index++) {
        if(resultArray[index]["deviceType"] == "USB_STORAGE") {
            pbnjson::JValue storageDriveList  = pbnjson::Array();
            pbnjson::JValue deviceInfoObj = pbnjson::Object();
            for(ssize_t idx = 0; idx < resultArray[index]["storageDriveList"].arraySize() ; idx++) {
                pbnjson::JValue driveListObj = pbnjson::Object();
                driveListObj.put("volumeLabel", resultArray[index]["storageDriveList"][idx]["volumeLabel"]);
                driveListObj.put("fsType", resultArray[index]["storageDriveList"][idx]["fsType"]);
                driveListObj.put("uuid", resultArray[index]["storageDriveList"][idx]["uuid"]);
                driveListObj.put("driveSize", resultArray[index]["storageDriveList"][idx]["driveSize"]);
                driveListObj.put("driveName", resultArray[index]["storageDriveList"][idx]["driveName"]);
                driveListObj.put("mountName", resultArray[index]["storageDriveList"][idx]["mountName"]);
                driveListObj.put("isMounted", resultArray[index]["storageDriveList"][idx]["isMounted"]);
                storageDriveList.put(idx,driveListObj);
            }
            deviceInfoObj.put("storageDriveList", storageDriveList);
            deviceInfoObj.put("hubPortPath", resultArray[index]["hubPortPath"]);
            deviceInfoObj.put("rootPath", resultArray[index]["rootPath"]);
            deviceInfoObj.put("deviceType", resultArray[index]["deviceType"]);
            deviceInfoObj.put("deviceSetId", resultArray[index]["deviceSetId"]);
            deviceInfoObj.put("deviceType", resultArray[index]["deviceType"]);
            deviceInfoObj.put("productId", resultArray[index]["productId"]);
            deviceInfoObj.put("vendorId", resultArray[index]["vendorId"]);
            deviceInfoObj.put("devPath", resultArray[index]["devPath"]);
            deviceInfoObj.put("productName", resultArray[index]["productName"]);
            deviceInfoObj.put("vendorName", resultArray[index]["vendorName"]);
            deviceInfoObj.put("serialNumber", resultArray[index]["serialNumber"]);
            deviceInfoObj.put("deviceNum", resultArray[index]["deviceNum"]);
            deviceInfoObj.put("deviceSubtype", resultArray[index]["deviceSubtype"]);
            deviceInfoObj.put("devSpeed", resultArray[index]["devSpeed"]);
            deviceInfoObj.put("errorReason", resultArray[index]["errorReason"]);
            storageDeviceList.put(storageDeviceCount, deviceInfoObj); storageDeviceCount ++;
        }
    }
    payload.put("storageDeviceList", storageDeviceList);
    return payload;
}

pbnjson::JValue PdmLunaService::getNonStorageDevicePayload(pbnjson::JValue resultArray) {

    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    pbnjson::JValue payload = pbnjson::Object();
    pbnjson::JValue nonStorageDeviceList = pbnjson::Array();
    int nonStorageDeviceCount = 0;
    for(ssize_t index = 0; index < resultArray.arraySize() ; ++index) {
        if(resultArray[index]["deviceType"] != "USB_STORAGE") {
            pbnjson::JValue deviceInfoObj = pbnjson::Object();
            deviceInfoObj.put("hubPortPath", resultArray[index]["hubPortPath"]);
            deviceInfoObj.put("deviceSetId", resultArray[index]["deviceSetId"]);
            deviceInfoObj.put("deviceType", resultArray[index]["deviceType"]);
            deviceInfoObj.put("productId", resultArray[index]["productId"]);
            deviceInfoObj.put("vendorId", resultArray[index]["vendorId"]);
            deviceInfoObj.put("deviceNum", resultArray[index]["deviceNum"]);
            deviceInfoObj.put("usbPortNum", resultArray[index]["usbPortNum"]);
            deviceInfoObj.put("serialNumber", resultArray[index]["serialNumber"]);
            deviceInfoObj.put("vendorName", resultArray[index]["vendorName"]);
            deviceInfoObj.put("productName", resultArray[index]["productName"]);
            deviceInfoObj.put("deviceSubtype", resultArray[index]["deviceSubtype"]);
            deviceInfoObj.put("isPowerOnConnect", resultArray[index]["isPowerOnConnect"]);
            deviceInfoObj.put("devSpeed", resultArray[index]["devSpeed"]);
            deviceInfoObj.put("devPath", resultArray[index]["devPath"]);
            deviceInfoObj.put("deviceName", resultArray[index]["deviceName"]);
            nonStorageDeviceList.put(nonStorageDeviceCount, deviceInfoObj); nonStorageDeviceCount ++;
        }
    }
    payload.put("nonStorageDeviceList", nonStorageDeviceList);
    return payload;
}

bool PdmLunaService::createToast(const std::string &message, const std::string &iconUrl, std::string deviceSetId)
{
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    bool retValue = false;

    char formattedString[255];
    snprintf(formattedString, sizeof(formattedString),"%s\0", message.c_str());
    std::string formattedMessage(formattedString);
    pbnjson::JObject params = pbnjson::JObject();
    params.put("message", pbnjson::JValue(formattedMessage));
    params.put("sourceId", "com.webos.service.pdm");

    if (iconUrl.length() > 0)
    {
        params.put("iconUrl", pbnjson::JValue(iconUrl));
    }

    std::string sessionId = m_sessionMap[deviceSetId];
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d deviceSetId:%s, sessionId: %s", __FUNCTION__, __LINE__, deviceSetId.c_str(), sessionId.c_str());
    retValue = LSCallSessionOneReply(mServiceHandle,
        "luna://com.webos.notification/createToast",
        params.stringify().c_str(),
        sessionId.c_str(),
        NULL, NULL, NULL, &lserror);

    if(!retValue)
    {
        PDM_LOG_ERROR("Notification: %s line : %d error on LSCallOneReply for createToast",__FUNCTION__, __LINE__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return retValue;
}

bool PdmLunaService::mountDeviceToSession(std::string mountName, std::string driveName, std::string deviceSetId, std::string fsType)
{
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d mountName:%s driveName:%s deviceSetId:%s fsType:%s", __FUNCTION__, __LINE__,mountName.c_str(), driveName.c_str(),deviceSetId.c_str(), fsType.c_str());

    std::string drivePath("/dev/");
    drivePath.append(driveName);

    uid_t uid = 0;
    gid_t gid = 0;

    if (deviceSetId == "AVN")
    {
        PDM_LOG_DEBUG("PdmLunaService::%s line:%d Get the uid and gid for AVN user: %s for drive:%s", __FUNCTION__, __LINE__,avnUserId.c_str(), driveName.c_str());
        uid = PdmUtils::get_uid(avnUserId.c_str());
        gid = PdmUtils::get_gid(avnUserId.c_str());
    }
    else if (deviceSetId == "RSE-L")
    {
        PDM_LOG_DEBUG("PdmLunaService::%s line:%d Get the uid and gid for RSE-L user: %s for drive:%s", __FUNCTION__, __LINE__,rselUserId.c_str(), driveName.c_str());
        uid = PdmUtils::get_uid(rselUserId.c_str());
        gid = PdmUtils::get_gid(rselUserId.c_str());
    }

    else if (deviceSetId == "RSE-R")
    {
        PDM_LOG_DEBUG("PdmLunaService::%s line:%d Get the uid and gid for RSE-R user: %s for drive:%s", __FUNCTION__, __LINE__,rserUserId.c_str(), driveName.c_str());
        uid = PdmUtils::get_uid(rserUserId.c_str());
        gid = PdmUtils::get_gid(rserUserId.c_str());
    }

    uint64_t mountFlag = MS_MGC_VAL;
    mountFlag |= MS_RELATIME;

    ostringstream stru, strg;
    stru << uid;
    strg << gid;
    string uid_str = stru.str();
    string gid_str = strg.str();
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d UID: %s GID: %s", __FUNCTION__, __LINE__, uid_str.c_str(), gid_str.c_str());

    std::string data = ""; // NOFS
    if (fsType == "vfat")
        data = "shortname=mixed,uid=" + uid_str + ",gid=" + gid_str + ",umask=0002"; // FAT
    else if (fsType == "ntfs")
        data = "uid=" + uid_str + ",gid=" + gid_str + ",umask=0002"; // NTFS
    else if (fsType == "tntfs")
        data = "nls=utf8,max_prealloc_size=64m,uid=" + uid_str + ",gid=" + gid_str + ",umask=0002"; // TNTFS
    else if (fsType == "tfat")
        data = "iocharset=utf8,fastmount=1,max_prealloc_size=32m,uid=" + uid_str + ",gid=" + gid_str + ",umask=0002"; // TFAT

    int32_t ret = mount(drivePath.c_str(), mountName.c_str(), fsType.c_str(), mountFlag, (void*)data.c_str());
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d Drive path: %s, Mount name: %s, fs type: %s, data: %s", __FUNCTION__, __LINE__, drivePath.c_str(), mountName.c_str(), fsType.c_str(), data);
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d Mount output: %d", __FUNCTION__, __LINE__, ret);

    if(ret) {
        PDM_LOG_WARNING("PdmLunaService:%s line:%d mount failed. errno: %d strerror: %s", __FUNCTION__, __LINE__, errno, strerror(errno));
        return false;
    }
    std::string rootPath = "/tmp/usb/" + driveName.substr(0, 3);
    if (deviceSetId == "AVN")
    {
        PDM_LOG_DEBUG("PdmLunaService:%s line:%d Changing the owner for rootPath:%s driveName:%s to AVN user: %s", __FUNCTION__, __LINE__,rootPath.c_str(), driveName.c_str(), avnUserId.c_str());
        PdmUtils::do_chown(rootPath.c_str(), avnUserId.c_str(), avnUserId.c_str());
        PdmUtils::do_chown(mountName.c_str(), avnUserId.c_str(), avnUserId.c_str());
        if(0 != chmod(rootPath.c_str(), MOUNT_FILE_MODE)) {
            PDM_LOG_ERROR("PdmLunaService:%s line:%d chmod error: %d stderror: %s", __FUNCTION__, __LINE__, errno, strerror(errno));
        }
    }
    else if (deviceSetId == "RSE-L")
    {
        PDM_LOG_DEBUG("PdmLunaService:%s line:%d Changing the owner for rootPath:%s driveName:%s to RSE-L user: %s", __FUNCTION__, __LINE__,rootPath.c_str(), driveName.c_str(), rselUserId.c_str());
        PdmUtils::do_chown(rootPath.c_str(), rselUserId.c_str(), rselUserId.c_str());
        PdmUtils::do_chown(mountName.c_str(), rselUserId.c_str(), rselUserId.c_str());
        if (0 != chmod(rootPath.c_str(), MOUNT_FILE_MODE)) {
            PDM_LOG_ERROR("PdmLunaService:%s line:%d chmod error: %d stderror: %s", __FUNCTION__, __LINE__, errno, strerror(errno));
        }
    }
    else if (deviceSetId == "RSE-R")
    {
        PDM_LOG_DEBUG("PdmLunaService:%s line:%d Changing the owner for rootPath:%s driveName:%s to RSE-R user: %s", __FUNCTION__, __LINE__,rootPath.c_str(), driveName.c_str(), rserUserId.c_str());
        PdmUtils::do_chown(rootPath.c_str(), rserUserId.c_str(), rserUserId.c_str());
        PdmUtils::do_chown(mountName.c_str(), rserUserId.c_str(), rserUserId.c_str());
        if(0 != chmod(rootPath.c_str(), MOUNT_FILE_MODE)) {
            PDM_LOG_ERROR("PdmLunaService:%s line:%d chmod error: %d stderror: %s", __FUNCTION__, __LINE__, errno, strerror(errno));
        }
    }
    return true;
}

bool PdmLunaService::notifyToDisplay(pbnjson::JValue deviceList, std::string deviceSetId, std::string deviceType) {
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d deviceSetId:%s deviceType:%s", __FUNCTION__, __LINE__,deviceSetId.c_str(), deviceType.c_str() );
    bool bRetVal = false;
    LSError error;
    LSErrorInit(&error);
    deviceList.put("returnValue", true);
    if(deviceType== "USB_STORAGE") {
        if (deviceSetId == "AVN") {
            bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_DEVICES_AVN, deviceList.stringify(NULL).c_str(), &error);
        }
        if (deviceSetId == "RSE-L") {
            bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_DEVICES_RSE_L, deviceList.stringify(NULL).c_str(), &error);
        }
        if (deviceSetId == "RSE-R") {
            bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_DEVICES_RSE_R, deviceList.stringify(NULL).c_str(), &error);
        }
        if(deviceSetId == "HOST") {
            bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_STORAGE_DEVICES, deviceList.stringify(NULL).c_str(), &error);
        }
    }

    if(deviceType != "USB_STORAGE") {
        if (deviceSetId == "AVN") {
            bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_NON_STORAGE_DEVICES_AVN, deviceList.stringify(NULL).c_str(), &error);
        }
        if (deviceSetId == "RSE-L") {
            bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_NON_STORAGE_DEVICES_RSE_L, deviceList.stringify(NULL).c_str(), &error);
        }
        if (deviceSetId =="RSE-R") {
            bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_NON_STORAGE_DEVICES_RSE_R, deviceList.stringify(NULL).c_str(), &error);
        }
        if(deviceSetId == "HOST") {
            bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_NON_STORAGE_DEVICES, deviceList.stringify(NULL).c_str(), &error);
        }
    }
   LSERROR_CHECK_AND_PRINT(bRetVal, error);
   return bRetVal;
}

bool PdmLunaService::notifyAllDeviceToDisplay(std::string deviceSetId, pbnjson::JValue deviceList) {
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d deviceSetId:%s", __FUNCTION__, __LINE__,deviceSetId.c_str());
    bool bRetVal = false;
    LSError error;
    LSErrorInit(&error);
    deviceList.put("returnValue", true);

    if (deviceSetId == "AVN") {
        bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_ATTACHED_ALL_DEVICES_AVN, deviceList.stringify(NULL).c_str(), &error);
    }
    if (deviceSetId == "RSE-L") {
        bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_ATTACHED_ALL_DEVICES_RSE_L, deviceList.stringify(NULL).c_str(), &error);
    }
    if (deviceSetId == "RSE-R") {
        bRetVal = LSSubscriptionReply(mServiceHandle, PDM_EVENT_AUTO_ATTACHED_ALL_DEVICES_RSE_R, deviceList.stringify(NULL).c_str(), &error);
    }
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
    return bRetVal;
}

bool PdmLunaService::cbgetAttachedDeviceList(LSHandle *sh, LSMessage *message) {
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d payload:%s", __FUNCTION__, __LINE__, LSMessageGetPayload(message));
    LSError lserror;
    LSErrorInit(&lserror);
    LSMessageRef(message);
    bool subscribed = true;
    std::string errText;
    replyMsg = message;
    std::string session_id = LSMessageGetSessionId(message);
    std::string deviceSetId = "";
    if (session_id != "host")
    {
        auto sessionPair = std::find_if(std::begin(m_sessionMap), std::end(m_sessionMap), [&](const std::pair<std::string, std::string> &pair)
        {
            return (pair.second == session_id);
        });

        if (sessionPair != std::end(m_sessionMap))
        {
            deviceSetId = sessionPair->first;
        }
    }
    PDM_LOG_INFO("PdmLunaService:",0,"%s line: %d deviceSetId:%s", __FUNCTION__,__LINE__,deviceSetId.c_str());
    if (LSMessageIsSubscription(message)) {
        if (deviceSetId == "AVN") {
            subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_ATTACHED_ALL_DEVICES_AVN, message);
        }
        if (deviceSetId == "RSE-L") {
            subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_ATTACHED_ALL_DEVICES_RSE_L, message);
        }
        else if (deviceSetId == "RSE-R") {
            subscribed = subscriptionAdd(sh, PDM_EVENT_AUTO_ATTACHED_ALL_DEVICES_RSE_R, message);
        }
    }

    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue request;

    request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                              {"where", pbnjson::JArray{{{"prop", "deviceSetId"}, {"op", "="}, {"val", deviceSetId.c_str()}}}}};
    find_query.put("query", request);
    if (LSCallOneReply(sh,"luna://com.webos.service.db/find",
                      find_query.stringify().c_str(), cbDbResponse, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        LSMessageUnref(message);
    }
    return true;
}

bool PdmLunaService::cbDbResponse(LSHandle * sh, LSMessage * message, void * user_data)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    const char* payload = LSMessageGetPayload(message);
    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    if(request.isNull() || (!request["returnValue"].asBool()))
    {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Db8Response is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if (!object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d object is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    LSMessage* getToastReplyMsg = object->getReplyMsg();
    if (!getToastReplyMsg) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d getToastReplyMsg is empty ", __FUNCTION__, __LINE__);
    return false;
    }
    pbnjson::JValue deviceInfoArray = pbnjson::Array();
    pbnjson::JValue resultArray = request["results"];
    if(resultArray.isArray()) {
        if(resultArray.arraySize() == 0) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d No device Info in DB ", __FUNCTION__, __LINE__);
        }
    }
    pbnjson::JValue storageDeviceList =object->getStorageDevicePayload(resultArray);
    deviceInfoArray.put(0, storageDeviceList);
    pbnjson::JValue nonStorageDeviceList =object->getNonStorageDevicePayload(resultArray);
    deviceInfoArray.put(1, nonStorageDeviceList);
    pbnjson::JValue json = pbnjson::Object();
    json.put("returnValue", true);
    json.put("deviceListInfo", deviceInfoArray);
    if(!LSMessageReply( sh, getToastReplyMsg, json.stringify(NULL).c_str(), &lserror))
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        LSMessageUnref(message);
    }
    return true;
}

bool PdmLunaService::getDevicesFromDB(std::string deviceType, std::string sessionId)
{
    bool bRetVal;
    LSError error;
    LSErrorInit(&error);

    auto sessionPair = std::find_if(std::begin(m_sessionMap), std::end(m_sessionMap), [&](const std::pair<std::string, std::string> &pair)
    {
        return (pair.second == sessionId);
    });

    std::string deviceSetId = "";

    if (sessionPair != std::end(m_sessionMap))
    {
        deviceSetId = sessionPair->first;
    }
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d sessionId: %s, deviceSetId: %s", __FUNCTION__, __LINE__, sessionId.c_str(), deviceSetId.c_str());

    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue request;

    if (sessionId != "host")
    {
        if (deviceType == "USB_STORAGE")
        {
            isRequestForStorageDevice = true;
            PDM_LOG_DEBUG("PdmLunaService::%s line:%d deviceType: %s", __FUNCTION__, __LINE__, deviceType.c_str());
            request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                       {"where", pbnjson::JArray{{{"prop", "deviceSetId"}, {"op", "="}, {"val", deviceSetId.c_str()}},
                                                                 {{"prop", "deviceType"}, {"op", "="}, {"val", "USB_STORAGE"}}}}};
        }
        else
        {
            isRequestForStorageDevice =  false;
            request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                       {"where", pbnjson::JArray{{{"prop", "deviceSetId"}, {"op", "="}, {"val", deviceSetId.c_str()}},
                                                                 {{"prop", "deviceType"}, {"op", "!="}, {"val", "USB_STORAGE"}}}}};
        }
    }
    else
    {
        PDM_LOG_DEBUG("PdmLunaService::%s line:%d sessionId: %s", __FUNCTION__, __LINE__, sessionId.c_str());
        if (deviceType == "USB_STORAGE")
        {
            isRequestForStorageDevice = true;
            PDM_LOG_DEBUG("PdmLunaService::%s line:%d deviceType: %s", __FUNCTION__, __LINE__, deviceType.c_str());
            request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                       {"where", pbnjson::JArray{{{"prop", "deviceType"}, {"op", "="}, {"val", "USB_STORAGE"}}}}};
        }
        else
        {
            isRequestForStorageDevice = false;
            request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                       {"where", pbnjson::JArray{{{"prop", "deviceType"}, {"op", "!="}, {"val", "USB_STORAGE"}}}}};
        }
    }

    find_query.put("query", request);
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d find_query: %s", __FUNCTION__, __LINE__, find_query.stringify().c_str());
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/find",
                 find_query.stringify().c_str(), cbDb8FindResponse, this, NULL, &error) == false)
    {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&error, stderr);
        LSErrorFree(&error);
        return false;
    }
    return true;
}

bool PdmLunaService::cbDb8FindResponse(LSHandle * sh, LSMessage * message, void * user_data)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    const char* payload = LSMessageGetPayload(message);

    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    if(request.isNull() || (!request["returnValue"].asBool()))
    {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Db8Response is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if (!object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d object is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    LSMessage* getDevListReplyMsg = object->getReplyMsg();
    if (!getDevListReplyMsg) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d getDevListReplyMsg is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue deviceInfoArray = pbnjson::Array();
    pbnjson::JValue devicePayload = pbnjson::Object();
    pbnjson::JValue resultArray = request["results"];
    if(resultArray.isArray()) {
        if(resultArray.arraySize() == 0) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d No device Info in DB ", __FUNCTION__, __LINE__);
            if(isRequestForStorageDevice) {
                devicePayload = object->getStorageDevicePayload(resultArray);
            } else {
                devicePayload =object->getNonStorageDevicePayload(resultArray);
            }
        }
        else {
            if (resultArray[0]["deviceType"] == "USB_STORAGE") {
                PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
                devicePayload = object->getStorageDevicePayload(resultArray);
            }
            else {
                PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
                devicePayload =object->getNonStorageDevicePayload(resultArray);
            }
        }
    }
    deviceInfoArray.put(0, devicePayload);
    pbnjson::JValue json = pbnjson::Object();
    json.put("returnValue", true);
    json.put("deviceListInfo", deviceInfoArray);

    if(!LSMessageReply( sh, getDevListReplyMsg, json.stringify().c_str(), &lserror))
    {
        return false;
    }
    return true;
}

bool PdmLunaService::queryDevice(std::string hubPortPath)
{
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue request;
    request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                              {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};
    find_query.put("query", request);
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/find",
                             find_query.stringify().c_str(), cbQueryResponse, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        return false;
    }
    return true;
}

bool PdmLunaService::cbQueryResponse(LSHandle * sh, LSMessage * message, void * user_data) {

    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    const char* payload = LSMessageGetPayload(message);
    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    if(request.isNull() || (!request["returnValue"].asBool()))
    {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Db8Response is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if(nullptr == object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d PdmLunaService obj is NULL", __FUNCTION__, __LINE__);
        return false;
    }

    pbnjson::JValue resultArray = request["results"];
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d list:%s", __FUNCTION__, __LINE__, resultArray.stringify().c_str());
    if(resultArray.isArray()) {
        if(resultArray.arraySize() == 0) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d No device Info in DB ", __FUNCTION__, __LINE__);
            return false;
        }
        if((object) &&(!object->deleteAndUpdatePayload(resultArray))) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d unable to delete from db", __FUNCTION__, __LINE__);
        }
    }
    return true;
}

bool PdmLunaService::deleteAndUpdatePayload(pbnjson::JValue resultArray) {

    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    std::string hubPortPath = resultArray[0]["hubPortPath"].asString();
    m_deviceSetId = resultArray[0]["deviceSetId"].asString();
    m_deviceType =  resultArray[0]["deviceType"].asString();
    std::string rootPath = resultArray[0]["rootPath"].asString();

    PDM_LOG_INFO("PdmLunaService:",0,"%s line: %d frm db hubPortPath:%s m_deviceSetId:%s m_deviceType:%s", __FUNCTION__,__LINE__,hubPortPath.c_str(), m_deviceSetId.c_str(), m_deviceType.c_str());
    if (m_deviceType == "USB_STORAGE") {
        for(ssize_t idx = 0; idx < resultArray[0]["storageDriveList"].arraySize() ; idx++) {
            std::string mountName = resultArray[0]["storageDriveList"][idx]["mountName"].asString();
            PDM_LOG_DEBUG("PdmLunaService::%s line:%d mountName: %s", __FUNCTION__, __LINE__, mountName.c_str());
            if(umount(mountName)) {
               bool ret = PdmUtils::removeDirRecursive(mountName);
               if(!ret) {
                   PDM_LOG_DEBUG("PdmLunaService::%s line:%d unable to remove the mountName: %s", __FUNCTION__, __LINE__, mountName.c_str());
                }
            }
        }
        bool ret = PdmUtils::removeFile(rootPath);
        if(!ret) {
            PDM_LOG_DEBUG("PdmLunaService::%s line:%d unable to remove the rootPath :%s", __FUNCTION__, __LINE__, rootPath.c_str());
        }
    }
    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue request;
    request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                              {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};
    find_query.put("query", request);
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/del",
                             find_query.stringify().c_str(), cbDeleteResponse, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        return false;
    }
    if (m_deviceType == "USB_STORAGE") {
        displayDisconnectedToast("Storage", m_deviceSetId);
    }else {
        displayDisconnectedToast(m_deviceType, m_deviceSetId);
    }
    return true;
}

bool PdmLunaService::cbDeleteResponse(LSHandle * sh, LSMessage * message, void * user_data) {
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    const char* payload = LSMessageGetPayload(message);
    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if(nullptr == object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d PdmLunaService obj is NULL", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d request:%s", __FUNCTION__, __LINE__, request.stringify().c_str());
    if(request.isNull() || (!request["returnValue"].asBool()) || (!request["count"].asNumber<int>()))
    {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Not able to delete ", __FUNCTION__, __LINE__);
    } else {
        if((object) && !(object->updatePayload(m_deviceSetId, m_deviceType))) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d not able to update the payload ", __FUNCTION__, __LINE__);
        }
    }
    return true;
}

bool PdmLunaService::updatePayload(std::string deviceSetId, std::string deviceType) {
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue request;
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d deviceSetId: %s deviceType: %s", __FUNCTION__, __LINE__, deviceSetId.c_str(),deviceType.c_str());
    if (deviceType == "USB_STORAGE") {
        request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                       {"where", pbnjson::JArray{{{"prop", "deviceSetId"}, {"op", "="}, {"val", deviceSetId.c_str()}},
                                                                 {{"prop", "deviceType"}, {"op", "="}, {"val", "USB_STORAGE"}}}}};
    }else {
        request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                    {"where", pbnjson::JArray{{{"prop", "deviceSetId"}, {"op", "="}, {"val", deviceSetId.c_str()}},
                                                                 {{"prop", "deviceType"}, {"op", "!="}, {"val", "USB_STORAGE"}}}}};
    }
    find_query.put("query", request);
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/find",
                             find_query.stringify().c_str(), cbPayloadResponse, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        return false;
    }
    updateHostPayload(deviceType); // This is for host on removal of device
    updateAllDeviceSessionPayload(deviceSetId); //This is for getAttachedDeviceList for session on removal of device
    return true;
}

bool PdmLunaService::cbPayloadResponse(LSHandle * sh, LSMessage * message, void * user_data) {
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    const char* payload = LSMessageGetPayload(message);

    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    if(request.isNull() || (!request["returnValue"].asBool()))
    {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Db8Response is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if (!object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d object is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue deviceInfoArray = pbnjson::Array();
    pbnjson::JValue devicePayload = pbnjson::Object();
    pbnjson::JValue resultArray = request["results"];
    if(resultArray.isArray()) {
        if(resultArray.arraySize() == 0) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d No device Info in DB ", __FUNCTION__, __LINE__);
            pbnjson::JValue json = pbnjson::Object();
            if(m_deviceType == "USB_STORAGE") {
                devicePayload = object->getStorageDevicePayload(resultArray);
                deviceInfoArray.put(0,devicePayload);
                json.put("deviceListInfo", deviceInfoArray);
                std::string deviceSetId = m_deviceSetId;
                object->notifyToDisplay(json, deviceSetId,"USB_STORAGE" );
            } else {
                devicePayload = object->getNonStorageDevicePayload(resultArray);
                deviceInfoArray.put(0,devicePayload);
                json.put("deviceListInfo", deviceInfoArray);
                std::string deviceSetId = m_deviceSetId;
                object->notifyToDisplay(json, deviceSetId,"NON_STORAGE");
            }
        }
        else {
            if (resultArray[0]["deviceType"] == "USB_STORAGE") {
                PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
                devicePayload = object->getStorageDevicePayload(resultArray);
                pbnjson::JValue json = pbnjson::Object();
                deviceInfoArray.put(0,devicePayload);
                json.put("deviceListInfo", deviceInfoArray);
                std::string deviceSetId = m_deviceSetId;
                object->notifyToDisplay(json, deviceSetId,"USB_STORAGE" );
            }
            else {
                PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
                devicePayload =object->getNonStorageDevicePayload(resultArray);
                pbnjson::JValue json = pbnjson::Object();
                deviceInfoArray.put(0,devicePayload);
                json.put("deviceListInfo", deviceInfoArray);
                std::string deviceSetId = m_deviceSetId;
                object->notifyToDisplay(json, deviceSetId,"NON_STORAGE");
            }
        }
    }
    return true;
}

void PdmLunaService::updateHostPayload(std::string deviceType) {
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue request;
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d deviceType: %s", __FUNCTION__, __LINE__, deviceType.c_str());
    if (deviceType == "USB_STORAGE") {
        request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                       {"where", pbnjson::JArray{{{"prop", "deviceType"}, {"op", "="}, {"val", "USB_STORAGE"}}}}};
    }else {
        request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                    {"where", pbnjson::JArray{{{"prop", "deviceType"}, {"op", "!="}, {"val", "USB_STORAGE"}}}}};
    }
    find_query.put("query", request);
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/find",
                             find_query.stringify().c_str(), cbHostPayloadResponse, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

bool PdmLunaService::cbHostPayloadResponse(LSHandle * sh, LSMessage * message, void * user_data) {

    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    const char* payload = LSMessageGetPayload(message);

    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    if(request.isNull() || (!request["returnValue"].asBool()))
    {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Db8Response is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if (!object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d object is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue deviceInfoArray = pbnjson::Array();
    pbnjson::JValue devicePayload = pbnjson::Object();
    pbnjson::JValue resultArray = request["results"];
    if(resultArray.isArray()) {
        if(resultArray.arraySize() == 0) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d No device Info in DB ", __FUNCTION__, __LINE__);
            pbnjson::JValue json = pbnjson::Object();
            if(m_deviceType == "USB_STORAGE") {
                devicePayload = object->getStorageDevicePayload(resultArray);
                deviceInfoArray.put(0,devicePayload);
                json.put("deviceListInfo", deviceInfoArray);
                std::string deviceSetId = "HOST";
                object->notifyToDisplay(json, deviceSetId,"USB_STORAGE" );
            } else {
                devicePayload = object->getNonStorageDevicePayload(resultArray);
                deviceInfoArray.put(0,devicePayload);
                json.put("deviceListInfo", deviceInfoArray);
                std::string deviceSetId = "HOST";
                object->notifyToDisplay(json, deviceSetId,"NON_STORAGE");
            }
        }
        else {
            if (resultArray[0]["deviceType"] == "USB_STORAGE") {
                PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
                devicePayload = object->getStorageDevicePayload(resultArray);
                pbnjson::JValue json = pbnjson::Object();
                deviceInfoArray.put(0,devicePayload);
                json.put("deviceListInfo", deviceInfoArray);
                std::string deviceSetId = "HOST";
                object->notifyToDisplay(json, deviceSetId,"USB_STORAGE" );
            } else {
                PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
                devicePayload =object->getNonStorageDevicePayload(resultArray);
                pbnjson::JValue json = pbnjson::Object();
                deviceInfoArray.put(0,devicePayload);
                json.put("deviceListInfo", deviceInfoArray);
                std::string deviceSetId = "HOST";
                object->notifyToDisplay(json, deviceSetId,"NON_STORAGE");
            }
        }
    }
    return true;
}

void PdmLunaService::updateAllDeviceSessionPayload(std::string deviceSetId) {
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d deviceSetId:%s", __FUNCTION__, __LINE__,deviceSetId.c_str());
    LSError lserror;
    LSErrorInit(&lserror);
    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue request;
    request = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                                    {"where", pbnjson::JArray{{{"prop", "deviceSetId"}, {"op", "="}, {"val", deviceSetId.c_str()}}}}};
    find_query.put("query", request);
    if (LSCallOneReply(mServiceHandle,"luna://com.webos.service.db/find",
                             find_query.stringify().c_str(), cbAllDeviceSessionResponse, this, NULL, &lserror) == false) {
        PDM_LOG_DEBUG("finding to the db failed in %s", __PRETTY_FUNCTION__ );
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
}

bool PdmLunaService::cbAllDeviceSessionResponse(LSHandle * sh, LSMessage * message, void * user_data) {
    PDM_LOG_DEBUG("PdmLunaService:%s line: %d", __FUNCTION__, __LINE__);
    const char* payload = LSMessageGetPayload(message);

    if(!payload) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d payload is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue request = pbnjson::JDomParser::fromString(payload);
    if(request.isNull() || (!request["returnValue"].asBool()))
    {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d Db8Response is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    PdmLunaService* object = (PdmLunaService*)user_data;
    if (!object) {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d object is empty ", __FUNCTION__, __LINE__);
        return false;
    }
    pbnjson::JValue deviceInfoArray = pbnjson::Array();
    pbnjson::JValue devicePayload = pbnjson::Object();
    pbnjson::JValue resultArray = request["results"];
    pbnjson::JValue storageDeviceList;
    pbnjson::JValue nonStorageDeviceList;
    if(resultArray.isArray()) {
        if(resultArray.arraySize() == 0) {
            PDM_LOG_ERROR("PdmLunaService:%s line: %d No device Info in DB ", __FUNCTION__, __LINE__);
            storageDeviceList =object->getStorageDevicePayload(resultArray);
            deviceInfoArray.put(0, storageDeviceList);
            nonStorageDeviceList =object->getNonStorageDevicePayload(resultArray);
            deviceInfoArray.put(1, nonStorageDeviceList);
            pbnjson::JValue json = pbnjson::Object();
            json.put("returnValue", "true");
            json.put("deviceListInfo", deviceInfoArray);
            std::string deviceSetId = m_deviceSetId;
            object->notifyAllDeviceToDisplay(deviceSetId,json);
        }
        else {
            storageDeviceList =object->getStorageDevicePayload(resultArray);
            deviceInfoArray.put(0, storageDeviceList);
            nonStorageDeviceList =object->getNonStorageDevicePayload(resultArray);
            deviceInfoArray.put(1, nonStorageDeviceList);
            pbnjson::JValue json = pbnjson::Object();
            json.put("returnValue", true);
            json.put("deviceListInfo", deviceInfoArray);
            std::string deviceSetId = m_deviceSetId;
            object->notifyAllDeviceToDisplay(deviceSetId,json);
        }
    }
    return true;
}

void PdmLunaService::deleteDeviceFromDb(std::string deviceSetId)
{
    PDM_LOG_INFO("PdmLunaService:",0,"%s line: %d deviceSetId:%s", __FUNCTION__,__LINE__, deviceSetId.c_str());

    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue query;
    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                             {"where", pbnjson::JArray{{{"prop", "deviceSetId"}, {"op", "="}, {"val", deviceSetId.c_str()}}}}};

    find_query.put("query", query);

    LS::Payload find_payload(find_query);
    LS::Call call = LunaIPC::getInstance()->getLSCPPHandle()->callOneReply("luna://com.webos.service.db/del", find_payload.getJson(), NULL, this, NULL);
    LS::Message message = call.get();

    LS::PayloadRef response_payload = message.accessPayload();
    pbnjson::JValue request = response_payload.getJValue();

    if(request.isNull())
    {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d reuest is empty ", __FUNCTION__, __LINE__);
    }

    if(!request["returnValue"].asBool())
    {
        PDM_LOG_ERROR("PdmLunaService::%s line: %d not able to delete from db for %s",  __FUNCTION__, __LINE__,deviceSetId.c_str());
    }
}

void PdmLunaService::deletePreviousMountName(std::string hubPortPath)
{
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d hubPortPath:%s ", __FUNCTION__, __LINE__, hubPortPath.c_str());
    LSError lserror;
    LSErrorInit(&lserror);

    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue query;
    int32_t driveSize = 0;

    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                             {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};

    find_query.put("query", query);

    LS::Payload find_payload(find_query);
    LS::Call call = LunaIPC::getInstance()->getLSCPPHandle()->callOneReply("luna://com.webos.service.db/find", find_payload.getJson(), NULL, this, NULL);
    LS::Message message = call.get();

    LS::PayloadRef response_payload = message.accessPayload();
    pbnjson::JValue request = response_payload.getJValue();

    if(request.isNull())
    {
        PDM_LOG_DEBUG("Db8 LS2 response is empty in %s", __PRETTY_FUNCTION__ );
    }

    if(!request["returnValue"].asBool())
    {
        PDM_LOG_DEBUG("Call to Db8 to is failed in %s", __PRETTY_FUNCTION__ );
    }

    if (request["results"][0]["deviceType"] == "USB_STORAGE")
    {
        for(ssize_t idx = 0; idx < request["results"][0]["storageDriveList"].arraySize() ; idx++) {
            std::string mountName = request["results"][0]["storageDriveList"][idx]["mountName"].asString();
            PDM_LOG_DEBUG("DiskPartitionInfo::%s line:%d driveName:%s", __FUNCTION__, __LINE__,mountName.c_str());
            if(umount(mountName)) {
                int ret = PdmUtils::removeDirRecursive(mountName);
                if(ret) {
                    PDM_LOG_DEBUG("PdmLunaService::%s line:%d removed the directory: %s", __FUNCTION__, __LINE__, mountName.c_str());
                }
            }
        }
    }
}

void PdmLunaService::deletePreviousPayload(std::string hubPortPath)
{
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d hubPortPath:%s ", __FUNCTION__, __LINE__, hubPortPath.c_str());
    LSError lserror;
    LSErrorInit(&lserror);

    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue query;
    int32_t driveSize = 0;

    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                             {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};

    find_query.put("query", query);

    LS::Payload find_payload(find_query);
    LS::Call call = LunaIPC::getInstance()->getLSCPPHandle()->callOneReply("luna://com.webos.service.db/del", find_payload.getJson(), NULL, this, NULL);
    LS::Message message = call.get();

    LS::PayloadRef response_payload = message.accessPayload();
    pbnjson::JValue request = response_payload.getJValue();

    if(request.isNull())
    {
        PDM_LOG_ERROR("PdmLunaService::%s:%d Db8 LS2 response is empty", __FUNCTION__, __LINE__);
    }

    if(!request["returnValue"].asBool())
    {
        PDM_LOG_ERROR("PdmLunaService::%s:%d NOT able to delete from db", __FUNCTION__, __LINE__);
    }
    PDM_LOG_INFO("PdmLunaService:",0,"%s line: %d deleted the previous set device", __FUNCTION__,__LINE__);
}

void PdmLunaService::displayConnectedToast(std::string device, std::string deviceSetId)
{
    PDM_LOG_INFO("PdmLunaService:",0,"%s line: %d device : %s deviceSetId: %s", __FUNCTION__,__LINE__,device.c_str(), deviceSetId.c_str());
    createToast( device + " device is connected", DEVICE_CONNECTED_ICON_PATH, deviceSetId);
}

void PdmLunaService::displayDisconnectedToast(std::string device, std::string deviceSetId)
{
    PDM_LOG_INFO("PdmLunaService:",0,"%s line: %d device : %s deviceSetId: %s", __FUNCTION__,__LINE__,device.c_str(), deviceSetId.c_str());
    createToast( device + " device is disconnected", DEVICE_CONNECTED_ICON_PATH, deviceSetId);
}
#endif
