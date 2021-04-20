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

#ifndef _PDMLUNASERVICE_H
#define _PDMLUNASERVICE_H

#include <map>
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

#define USER_MOUNT                            1

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
        static bool isRequestForStorageDevice;
        static std::string m_sessionId;
        static std::string m_deviceSetId;
        static std::string m_ejectedDeviceSetId;
        static std::string m_deviceType;
        static std::string avnUserId;
        static std::string rselUserId;
        static std::string rserUserId;
        static std::string m_requestedDrive;
        LS::Handle *mServiceCPPHandle;
        static LSMethod pdm_dev_methods[];
        static std::map<std::string, std::string> m_sessionMap;
        static std::map<std::string, std::string> m_portDisplayMap;
        std::map <std::string, std::string> m_hubPortPathSessionIdMap;
        LSMessage* replyMsg;
        LSMessage* getReplyMsg() { return replyMsg;}
        bool isGetSpaceInfoRequest;
        void deleteDeviceFromDbExceptBTDongle();
        void deleteDeviceFromDb(std::string deviceType);
        void updateGetAllDevicePayload(pbnjson::JValue list);
#endif
        CommandManager *mCommandManager;
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
#ifdef WEBOS_SESSION
        LS::Handle *get_LSCPPHandle();
#endif
        //callbacks
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
#ifdef WEBOS_SESSION
        static bool _cbgetAttachedDeviceList(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbgetAttachedDeviceList(sh, message);
        }
        static bool _cbgetAttachedAllDeviceList(LSHandle *sh, LSMessage *message , void *data){
             return static_cast<PdmLunaService*>(data)->cbgetAttachedAllDeviceList(sh, message);
        }
        static bool _cbSetDeviceForSession(LSHandle *sh, LSMessage *message , void *data){
            return static_cast<PdmLunaService*>(data)->cbSetDeviceForSession(sh, message);
        }
        std::string findPreviousSessionId(std::string hubPortPath);
        void deletePreviousPayload(std::string hubPortPath);
        void deletePreviousMountName(std::string hubPortPath);
        void displayConnectedToast(std::string device, std::string deviceSetId);
        void displayDisconnectedToast(std::string device, std::string deviceSetId);
        bool isDriveBusy(std::string mountName);
        bool isWritable(std::string driveNmae);
        void updateAllDeviceSessionPayload(std::string deviceSetId);
        void updateHostPayload(std::string deviceType);
        bool deleteAndUpdatePayload(pbnjson::JValue resultArray);
        bool updatePayload(std::string deviceSetId, std::string deviceType);
        bool queryDevice(std::string hubPortPath);
        bool cbSetDeviceForSession(LSHandle *sh, LSMessage *message);
        static bool cbDb8Response(LSHandle* lshandle, LSMessage *message, void *user_data);
        bool storeDeviceInfo(pbnjson::JValue list);
        bool updateIsMount(pbnjson::JValue list,std::string driveName);
        bool updateErrorReason(pbnjson::JValue list);
        bool ejectDevice(pbnjson::JValue list,PdmLunaService* object);
        bool umount(std::string mountPath);
        void findDevice(LSHandle * sh, int deviceNum);
        bool mountDeviceToSession(std::string mountName, std::string driveName, std::string deviceSetId, std::string fsType);
        bool createToast(const std::string &message, const std::string &iconUrl, std::string deviceSetId);
        bool queryForSession();
        void notifyResumeDone();
        bool notifyToDisplay(pbnjson::JValue deviceList, std::string deviceSetId, std::string deviceType);
        bool notifyAllDeviceToDisplay(std::string deviceSetId, pbnjson::JValue deviceList);
        void updateDeviceAlldeviceList();
        void updateDeviceList(std::string deviceSetId);
        void updateStorageDeviceList(std::string deviceSetId);
        pbnjson::JValue getStorageDevicePayload(pbnjson::JValue resultArray);
        pbnjson::JValue getNonStorageDevicePayload(pbnjson::JValue resultArray);
        static bool cbAllDeviceSessionResponse(LSHandle * sh, LSMessage * message, void * user_data);
        static bool cbHostPayloadResponse (LSHandle * sh, LSMessage * message, void * user_data);
        static bool cbPayloadResponse(LSHandle * sh, LSMessage * message, void * user_data);
        static bool cbQueryResponse(LSHandle* lshandle, LSMessage *message, void *user_data);
        static bool cbDb8FindResponse(LSHandle * sh, LSMessage * message, void * user_data);
        static bool cbDeleteResponse (LSHandle * sh, LSMessage * message, void * user_data);
        bool getDevicesFromDB(std::string deviceType, std::string sessionId);
        bool cbgetAttachedAllDeviceList(LSHandle *sh, LSMessage *message);
        pbnjson::JValue createJsonGetAttachedAllDeviceList(LSMessage *message );
        bool cbgetAttachedDeviceList(LSHandle *sh, LSMessage *message);
        static bool cbDbResponse(LSHandle * sh, LSMessage * message, void * user_data);
        static bool cbEjectDevice(LSHandle * sh, LSMessage * message, void * user_data);
        static bool cbFindDriveName(LSHandle * sh, LSMessage * message, void * user_data);
        static bool cbUpdateDeviceListResponse(LSHandle * sh, LSMessage * message, void * user_data);
        static bool cbUpdateStorageDeviceListResponse(LSHandle * sh, LSMessage * message, void * user_data);
#endif
        bool notifySubscribers(int eventDeviceType, const int &eventID, std::string hubPortPath);
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

        bool deinit();

};
#endif
