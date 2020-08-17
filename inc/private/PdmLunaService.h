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

#ifndef _PDMLUNASERVICE_H
#define _PDMLUNASERVICE_H

#include <string>
#include <glib.h>
#include <lunaservice.h>
#include <luna-service2++/handle.hpp>
#include <luna-service2/lunaservice.hpp>

#include "DeviceTracker.h"
#include "pbnjson.hpp"
#include "CommandTypes.h"
#include "Command.h"

#define PDM_SERVICE_NAME                  "com.webos.service.pdm"

#define PDM_EVENT_STORAGE_DEVICES            "getAttachedStorageDevicesList"
#define PDM_EVENT_NON_STORAGE_DEVICES        "getAttachedNonStorageDevicesList"
#define PDM_EVENT_ALL_ATTACHED_DEVICES       "getAllAttachedDevicesList"
#define PDM_EVENT_AUDIO_DEVICES              "getAttachedAudioDeviceList"

//#ifdef WEBOS_SESSION
#define PDM_EVENT_ALL_ATTACHED_DEVICE_LIST         "getAttachedAllDeviceList"
#define PDM_EVENT_AUTO_STORAGE_DEVICES             "getAttachedAutoStorageDeviceList"
#define PDM_EVENT_AUTO_NON_STORAGE_DEVICES         "getAttachedAutoNonStorageDeviceList"
#define PDM_EVENT_AUTO_DEVICES_AVN                 "getAttachedAutoDeviceList:AVN"
#define PDM_EVENT_AUTO_DEVICES_RSE_L               "getAttachedAutoDeviceList:RSE-L"
#define PDM_EVENT_AUTO_DEVICES_RSE_R               "getAttachedAutoDeviceList:RSE-R"
#define PDM_EVENT_AUTO_NON_STORAGE_DEVICES_AVN     "getAttachedAutoNonStorageDeviceList:AVN"
#define PDM_EVENT_AUTO_NON_STORAGE_DEVICES_RSE_L   "getAttachedAutoNonStorageDeviceList:RSE-L"
#define PDM_EVENT_AUTO_NON_STORAGE_DEVICES_RSE_R   "getAttachedAutoNonStorageDeviceList:RSE-R"
#define PDM_EVENT_AUTO_ATTACHED_ALL_DEVICES_AVN     "getAttachedAutoAllDeviceList:AVN"
#define PDM_EVENT_AUTO_ATTACHED_ALL_DEVICES_RSE_L   "getAttachedAutoAllDeviceList:RSE-L"
#define PDM_EVENT_AUTO_ATTACHED_ALL_DEVICES_RSE_R   "getAttachedAutoAllDeviceList:RSE-R"
//#endif

#define LSERROR_CHECK_AND_PRINT(ret, lsError)\
     do {                          \
         if (ret == false) {               \
             LSErrorPrintAndFree(&lsError);\
             return false; \
         }                 \
     } while (0)

class CommandManager;
class PdmLunaService
{
    private:
        LSHandle *mServiceHandle;
        void LSErrorPrintAndFree(LSError *ptrLSError);
        void appendErrorResponse(pbnjson::JValue &payload, int errorCode, std::string errorText);
        static LSMethod pdm_methods[];
#ifdef WEBOS_SESSION
        LS::Handle *mServiceCPPHandle;
        static LSMethod pdm_dev_methods[];
        static std::map<std::string, std::string> m_sessionMap;
#endif
        CommandManager *mCommandManager;
        LSMessage* replyMsg;
        LSMessage* getReplyMsg() { return replyMsg;}
        bool subscriptionAdd(LSHandle *a_sh, const char *a_key, LSMessage *a_message);
        pbnjson::JValue createJsonGetAttachedDeviceStatus(LSMessage *message);
        pbnjson::JValue createJsonGetAttachedNonStorageDeviceList(LSMessage *message);
        pbnjson::JValue createJsonGetAttachedStorageDeviceList(LSMessage *message);
        pbnjson::JValue createJsonGetAttachedAudioDeviceList(LSMessage *message);
    public:
        PdmLunaService(CommandManager *cmdManager);
        ~PdmLunaService();
        bool init(GMainLoop *);
        bool PdmLunaServiceRegister(const char *srvcname, GMainLoop *mainLoop, LSHandle **mServiceHandle);
        LSHandle *get_LSHandle(void);
        //callbacks
        static bool _cbgetAttachedDeviceList(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbgetAttachedDeviceList(sh, message);
        }
        static bool cbDbResponse(LSHandle * sh, LSMessage * message, void * user_data);
        static bool _cbgetExample(LSHandle *sh, LSMessage *message , void *data) {
            return static_cast<PdmLunaService*>(data)->cbGetExample(sh, message);
        }
        static bool _cbgetAttachedStorageDeviceList(LSHandle *sh, LSMessage *message , void *data) {
            return static_cast<PdmLunaService*>(data)->cbGetAttachedStorageDeviceList(sh, message);
        }
        static bool _cbgetAttachedNonStorageDeviceList(LSHandle *sh, LSMessage *message , void *data) {
            return static_cast<PdmLunaService*>(data)->cbGetAttachedNonStorageDeviceList(sh, message);
        }
        static bool _cbgetAttachedDeviceStatus(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbGetAttachedDeviceStatus(sh, message);
        }
        static bool _cbgetSpaceInfo(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbGetSpaceInfo(sh, message);
        }
        static bool _cbformat(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbFormat(sh, message);
        }
        static bool _cbfsck(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbFsck(sh, message);
        }
        static bool _cbeject(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbEject(sh, message);
        }
        static bool _cbsetVolumeLabel(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbSetVolumeLabel(sh, message);
        }
        static bool _cbisWritableDrive(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbIsWritableDrive(sh, message);
        }
        static bool _cbumountAllDrive(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbUmountAllDrive(sh, message);
        }
        static bool _cbmountandFullFsck(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbmountandFullFsck(sh, message);
        }
//#ifdef WEBOS_SESSION
        static bool _cbgetAttachedAllDeviceList(LSHandle *sh, LSMessage *message , void *data){
             return static_cast<PdmLunaService*>(data)->cbgetAttachedAllDeviceList(sh, message);
        }
#ifdef WEBOS_SESSION
	static bool _cbSetDeviceForSession(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbSetDeviceForSession(sh, message);
        }
        bool cbSetDeviceForSession(LSHandle *sh, LSMessage *message);
        static bool cbDb8Response(LSHandle* lshandle, LSMessage *message, void *user_data);
        bool storeDeviceInfo(pbnjson::JValue list);
        bool mountDeviceToSession(std::string driveName, std::string deviceSetId);
        bool createToast(const std::string &message, const std::string &iconUrl, std::string deviceSetId);
        bool queryForSession();
        bool notifyToDisplay(pbnjson::JValue deviceList, std::string deviceSetId, std::string deviceType);
        bool notifyAllDeviceToDisplay(std::string deviceSetId, pbnjson::JValue deviceList);
        pbnjson::JValue getStorageDevicePayload(pbnjson::JValue resultArray);
        pbnjson::JValue getNonStorageDevicePayload(pbnjson::JValue resultArray);
#endif
        bool notifySubscribers(int eventDeviceType);
        bool cbgetAttachedAllDeviceList(LSHandle *sh, LSMessage *message);
        pbnjson::JValue createJsonGetAttachedAllDeviceList(LSMessage *message );
        bool cbGetExample(LSHandle *sh, LSMessage *message);
        bool cbGetAttachedStorageDeviceList(LSHandle *sh, LSMessage *message);
        bool cbGetAttachedNonStorageDeviceList(LSHandle *sh, LSMessage *message);
        bool cbGetAttachedDeviceStatus(LSHandle *sh, LSMessage *message);
        bool cbGetSpaceInfo(LSHandle *sh, LSMessage *message);
        bool cbFormat(LSHandle *sh, LSMessage *message);
        bool cbFsck(LSHandle *sh, LSMessage *message);
        bool cbEject(LSHandle *sh, LSMessage *message);
        bool cbSetVolumeLabel(LSHandle *sh, LSMessage *message);
        bool cbIsWritableDrive(LSHandle *sh, LSMessage *message);
        bool cbUmountAllDrive(LSHandle *sh, LSMessage *message);
        bool commandReply(CommandResponse *cmdRes, void *msg);
        bool cbmountandFullFsck(LSHandle *sh, LSMessage *message);
        bool cbgetAttachedDeviceList(LSHandle *sh, LSMessage *message);

        bool deinit();

};
#endif
