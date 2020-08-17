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

#include <functional>

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

#ifdef WEBOS_SESSION
#define DB8_KIND "com.webos.service.pdmhistory:1"
std::map<std::string, std::string> PdmLunaService::m_sessionMap = {};
const std::string DEVICE_CONNECTED_ICON_PATH = "/usr/share/physical-device-manager/usb_connect.png";
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
    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_VALIDATE_ATTACH_DEVICE_LIST);
    pbnjson::JValue payload = createJsonGetAttachedStorageDeviceList(message);

    PDM_LOG_DEBUG("PdmLunaService::cbgetAttachedStorageDeviceList");

    if (LSMessageIsSubscription(message))
    {
        subscribed = subscriptionAdd(sh, PDM_EVENT_STORAGE_DEVICES, message);
    }

    payload.put("returnValue", subscribed);

    bRetVal  =  LSMessageReply (sh,  message,  payload.stringify(NULL).c_str() ,  &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);
    return true;
}

bool PdmLunaService::cbGetAttachedNonStorageDeviceList(LSHandle *sh, LSMessage *message)
{
    bool bRetVal;
    LSError error;
    LSErrorInit(&error);
    bool subscribed = false;
    VALIDATE_SCHEMA_AND_RETURN(sh, message, JSON_SCHEMA_VALIDATE_NON_STORAGE_ATTACH_DEVICE_LIST);
    pbnjson::JValue payload = createJsonGetAttachedNonStorageDeviceList(message);
    PDM_LOG_DEBUG("PdmLunaService::cbgetAttachedNonStorageDeviceList");
    if (LSMessageIsSubscription(message))
    {
        subscribed = subscriptionAdd(sh, PDM_EVENT_NON_STORAGE_DEVICES, message);
    }

    payload.put("returnValue", subscribed);

    bRetVal  =  LSMessageReply (sh,  message,  payload.stringify(NULL).c_str() ,  &error);
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
    SpaceInfoCommand *spaceCmd = new (std::nothrow) SpaceInfoCommand;

    if(!spaceCmd) {
         PDM_LOG_ERROR("PdmLunaService:%s line: %d SpaceInfoCommand ", __FUNCTION__, __LINE__);
         return true;
    }

    if(!payloadMsg) {
         PDM_LOG_ERROR("PdmLunaService:%s line: %d payloadMsg is empty ", __FUNCTION__, __LINE__);
         delete spaceCmd;
         spaceCmd = nullptr;
         return true;
    }

    pbnjson::JValue list = pbnjson::JDomParser::fromString(payloadMsg);
    spaceCmd->directCheck = list["directCheck"].asBool();
    spaceCmd->driveName = list["driveName"].asString();
    LSMessageRef(message);
    PdmCommand *cmdSpace = new (std::nothrow) PdmCommand(reinterpret_cast<CommandType*>(spaceCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );

    if(cmdSpace){
        mCommandManager->sendCommand(cmdSpace);
    } else {
        PDM_LOG_ERROR("PdmLunaService:%s line: %d cmdSpace creation failed ", __FUNCTION__, __LINE__);
    }

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

    EjectCommand *ejectCmd = new EjectCommand;
    if(!ejectCmd)
        return true;

    ejectCmd->deviceNumber = root["deviceNum"].asNumber<int>();
    LSMessageRef(message);
    PdmCommand *cmdeject = new PdmCommand(reinterpret_cast<CommandType*>(ejectCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );
    mCommandManager->sendCommand(cmdeject);
    return true;
}

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

        IsWritableCommand *writableCmd = new IsWritableCommand;
        if(!writableCmd)
            return true;

        writableCmd->driveName = list["driveName"].asString();
        LSMessageRef(message);
        PdmCommand *cmdIsWritable = new PdmCommand(reinterpret_cast<CommandType*>(writableCmd), std::bind(&PdmLunaService::commandReply, this, _1, _2),(void*)message );
        mCommandManager->sendCommand(cmdIsWritable);

    }

    return true;
}

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

bool PdmLunaService::notifySubscribers(int eventDeviceType)
{
    PDM_LOG_DEBUG("PdmLunaService::notifySubscribers - Device Type: %s",DeviceEventTable[eventDeviceType]);

    bool bRetVal = false;
    LSError error;
    LSErrorInit(&error);
    pbnjson::JValue payload ;
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

    if((eventDeviceType == NON_STORAGE_DEVICE) || (eventDeviceType == STORAGE_DEVICE)) {
         payload = createJsonGetAttachedAllDeviceList(nullptr);
         bRetVal = LSSubscriptionReply(mServiceHandle,PDM_EVENT_ALL_ATTACHED_DEVICE_LIST, payload.stringify(NULL).c_str(), &error);
         LSERROR_CHECK_AND_PRINT(bRetVal, error);
    }

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
    bool bRetVal, success;
    LSError error;
    LSErrorInit(&error);
    bool subscribed = false;
    std::string deviceSetId = "";
    pbnjson::JValue response = pbnjson::Object();
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

        if (!deviceSetId.empty() && (deviceSetId != "Select"))
            success = storeDeviceInfo(device);
        std::string storageDeviceType = device["deviceType"].asString();
        deviceSetId = device["deviceSetId"].asString();
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

        if (storageDeviceType == "USB_STORAGE") {
            createToast("Storage device is connected", DEVICE_CONNECTED_ICON_PATH, deviceSetId);
            for (auto& drive : device["storageDriveList"].items()) {
                std::string driveName = drive["driveName"].asString();
                std::string fsType = drive["fsType"].asString();
                if (fsType == "tntfs" || fsType == "ntfs" || fsType == "vfat" || fsType == "tfat") {
                    success = mountDeviceToSession(driveName, deviceSetId);
                    response.put("returnValue", success);
                }
            }
        }
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

        if (!deviceSetId.empty() && (deviceSetId != "Select"))
            success = storeDeviceInfo(device);
        response.put("returnValue", success);
        std::string nonStorageDeviceType = device["deviceType"].asString();
        deviceSetId = device["deviceSetId"].asString();
        if(deviceSetId == "RSE-L") {
            nsDeviceConnectedRseL = true;allDeviceConnectedRseL=true;
            rselDeviceArray.put(rselDeviceNo, device);
            hostDeviceArray.put(hostDeviceNo,device);
            rselAllDeviceArray.put(rselAllDeviceNo, device);
            rselDeviceNo++; hostDeviceNo++;rselAllDeviceNo++;
        }
        if(deviceSetId == "RSE-R") {
            nsDeviceConnectedRseR = true;allDeviceConnectedRseR = true;
            rserDeviceArray.put(rserDeviceNo, device);
            hostDeviceArray.put(hostDeviceNo,device);
            rserAllDeviceArray.put(rserAllDeviceNo, device);
            rserDeviceNo++; hostDeviceNo++;rserAllDeviceNo++;
        }

	if(deviceSetId == "AVN") {
            nsDeviceConnectedAvn = true; allDeviceConnectedAvn = true;
            avnDeviceArray.put(avnDeviceNo, device);
            hostDeviceArray.put(hostDeviceNo,device);
            avnAllDeviceArray.put(avnAllDeviceNo, device);
            avnDeviceNo++;hostDeviceNo++;avnAllDeviceNo++;
        }
        PDM_LOG_DEBUG("PdmLunaService:%s line: %d nonStorageDeviceType: %s deviceSetId: %s", __FUNCTION__, __LINE__, nonStorageDeviceType.c_str(), deviceSetId.c_str());

        if (nonStorageDeviceType == "HID") {
            createToast("HID device is connected", DEVICE_CONNECTED_ICON_PATH, deviceSetId);
        }
        else if (nonStorageDeviceType == "BLUETOOTH") {
            createToast("Bluetooth device is connected", DEVICE_CONNECTED_ICON_PATH, deviceSetId);
        }
        else if (nonStorageDeviceType == "CAM") {
            createToast("Camera device is connected", DEVICE_CONNECTED_ICON_PATH, deviceSetId);
        }
        else if (nonStorageDeviceType == "XPAD") {
            createToast("Gamepad device is connected", DEVICE_CONNECTED_ICON_PATH, deviceSetId);
        }
        else if (nonStorageDeviceType == "NFC") {
            createToast("NFC device is connected", DEVICE_CONNECTED_ICON_PATH, deviceSetId);
        }
        else
        {
            createToast("Unknown device is connected", DEVICE_CONNECTED_ICON_PATH, deviceSetId);
        }
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

    deviceSetId0 = request["sessionList"][0]["deviceSetInfo"]["deviceSetId"].asString();
    sessionId0 = request["sessionList"][0]["sessionId"].asString();
    m_sessionMap.insert(std::pair<std::string, std::string>(deviceSetId0, sessionId0));
    PDM_LOG_DEBUG("PdmLunaService: %s line: %d DeviceSetId0: %s, SessionId0: %s", __FUNCTION__, __LINE__, deviceSetId0.c_str(), sessionId0.c_str());

    deviceSetId1 = request["sessionList"][1]["deviceSetInfo"]["deviceSetId"].asString();
    sessionId1 = request["sessionList"][1]["sessionId"].asString();
    m_sessionMap.insert(std::pair<std::string, std::string>(deviceSetId1, sessionId1));
    PDM_LOG_DEBUG("PdmLunaService: %s line: %d DeviceSetId1: %s, SessionId1: %s", __FUNCTION__, __LINE__, deviceSetId1.c_str(), sessionId1.c_str());

    if(!request["sessionList"][2].isNull())
    {
        deviceSetId2 = request["sessionList"][2]["deviceSetInfo"]["deviceSetId"].asString();
        sessionId2 = request["sessionList"][2]["sessionId"].asString();
        m_sessionMap.insert(std::pair<std::string, std::string>(deviceSetId2, sessionId2));
        PDM_LOG_DEBUG("PdmLunaService: %s line: %d DeviceSetId2: %s, SessionId2: %s", __FUNCTION__, __LINE__, deviceSetId2.c_str(), sessionId2.c_str());
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

        if (list["deviceSetId"].asString() == "AVN")
        {
            for(ssize_t idx = 0; idx < list["storageDriveList"].arraySize() ; idx++) {
                std::string fsType = list["storageDriveList"][idx]["fsType"].asString();
                if (fsType == "tntfs" || fsType == "ntfs" || fsType == "vfat" || fsType == "tfat")
                {
                    std::string driveName = list["storageDriveList"][idx]["driveName"].asString();
                    std::string mountPath = "/tmp/usb_avn/" + driveName;
                    list["storageDriveList"][idx].put("mountPath", mountPath);
                }
                else
                {
                    list["storageDriveList"][idx].put("mountPath", "UNSUPPORTED_FILESYSTEM");
                }
            }
        }
	else if (list["deviceSetId"].asString() == "RSE-L")
        {
            for(ssize_t idx = 0; idx < list["storageDriveList"].arraySize() ; idx++) {
                std::string fsType = list["storageDriveList"][idx]["fsType"].asString();
                if (fsType == "tntfs" || fsType == "ntfs" || fsType == "vfat" || fsType == "tfat")
                {
                    std::string driveName = list["storageDriveList"][idx]["driveName"].asString();
                    std::string mountPath = "/tmp/usb_rse_left/" + driveName;
                    list["storageDriveList"][idx].put("mountPath", mountPath);
                }
                else
                {
                    list["storageDriveList"][idx].put("mountPath", "UNSUPPORTED_FILESYSTEM");
                }
            }
        }
        else if (list["deviceSetId"].asString() == "RSE-R")
        {
            for(ssize_t idx = 0; idx < list["storageDriveList"].arraySize() ; idx++) {
                std::string fsType = list["storageDriveList"][idx]["fsType"].asString();
                if (fsType == "tntfs" || fsType == "ntfs" || fsType == "vfat" || fsType == "tfat")
                {
                    std::string driveName = list["storageDriveList"][idx]["driveName"].asString();
                    std::string mountPath = "/tmp/usb_rse_right/" + driveName;
                    list["storageDriveList"][idx].put("mountPath", mountPath);
                }
                else
                {
                    list["storageDriveList"][idx].put("mountPath", "UNSUPPORTED_FILESYSTEM");
                }
            }
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
                driveListObj.put("mountPath", resultArray[index]["storageDriveList"][idx]["mountPath"]);
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

bool PdmLunaService::mountDeviceToSession(std::string driveName, std::string deviceSetId)
{
    PDM_LOG_DEBUG("PdmLunaService::%s line:%d", __FUNCTION__, __LINE__);
    bool bRetVal = false;

    std::string driveDir("/dev/");
    driveDir.append(driveName);

    char command[255];

    if (deviceSetId == "AVN")
    {
        PDM_LOG_DEBUG("Mounting %s to AVN", driveName.c_str());
        sprintf (command, "/etc/pdm/scripts/mount_to_avn.sh %s\n", driveName.c_str());
    }
    else if (deviceSetId == "RSE-L")
    {
        PDM_LOG_DEBUG("Mounting %s to RSE-L", driveName.c_str());
        sprintf (command, "/etc/pdm/scripts/mount_to_rse_left.sh %s\n", driveName.c_str());
    }
    else if (deviceSetId == "RSE-R")
    {
        PDM_LOG_DEBUG("Mounting %s to RSE-R", driveName.c_str());
        sprintf (command, "/etc/pdm/scripts/mount_to_rse_right.sh %s\n", driveName.c_str());
    }
    system("chmod +x /etc/pdm/scripts/*");
    system(command);

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
#endif
