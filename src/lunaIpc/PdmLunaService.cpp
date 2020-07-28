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

const char* DeviceEventTable[] =
{
    PDM_EVENT_STORAGE_DEVICES,
    PDM_EVENT_NON_STORAGE_DEVICES,
    PDM_EVENT_ALL_ATTACHED_DEVICES,
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
    {NULL, NULL}
    };
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

    if (PdmLunaServiceRegister(PDM_SERVICE_NAME, mainLoop, &mServiceHandle) == false) {
        PDM_LOG_ERROR("com.webos.service.pdm service registration failed");
        return false;
    }
    PDM_LOG_DEBUG("mServiceHandle =%p", mServiceHandle);
    return true;
}

bool PdmLunaService::PdmLunaServiceRegister(const char *srvcname, GMainLoop *mainLoop, LSHandle **msvcHandle) {

    bool bRetVal = false;
    LSError error;
    LSErrorInit(&error);

    PDM_LOG_DEBUG("PdmLunaService::PdmLunaServiceRegister");

    //Register the service
    bRetVal = LSRegister(srvcname, msvcHandle, &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);

    //register category
    bRetVal = LSRegisterCategory(*msvcHandle, "/", pdm_methods, nullptr, nullptr, &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);

    bRetVal =  LSCategorySetData(*msvcHandle,  "/", this, &error);
    LSERROR_CHECK_AND_PRINT(bRetVal, error);

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
