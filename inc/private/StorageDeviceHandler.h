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

#ifndef STORAGEDEVICEHANDLER_H_
#define STORAGEDEVICEHANDLER_H_

#include "DeviceHandler.h"
#include "PdmDeviceFactory.h"
#include "PdmNetlinkEvent.h"
#include "StorageDevice.h"
#include "PdmLogUtils.h"
#include "DeviceClass.h"
#include <mutex>
#include <condition_variable>
#include <thread>

class HddDeviceHandler;

class StorageDeviceHandler: public DeviceHandler {
    friend class HddDeviceHandler;
private:


    bool mSpaceInfoThreadStatus;
    static bool mIsObjRegistered;
    std::size_t m_maxStorageDevices;
    std::list<StorageDevice*> mStorageList;
    std::thread mSpaceInfoThread;
    std::condition_variable mNotifyCv;
    std::mutex mNotifyMtx;
    std::mutex mStorageListMtx;

    StorageDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    //Register Object to object factory. This is called automatically
    static bool RegisterObject() {
           return (PdmDeviceFactory::getInstance()->Register("Storage",
                                              &StorageDeviceHandler::CreateObject));
    }
    void removeDevice(StorageDevice *storageDevice);
	bool deleteStorageDevice(DeviceClass*);
	void ProcessStorageDevice(DeviceClass*);
	void checkStorageDevice(DeviceClass*);
	void createStorageDevice(DeviceClass*, IDevice*);

	//bool deleteStorageDevice(PdmNetlinkEvent* pNE);
    //void ProcessStorageDevice(PdmNetlinkEvent* pNE);
    //void checkStorageDevice(PdmNetlinkEvent* pNE);
    //void createStorageDevice(PdmNetlinkEvent* pNE, IDevice *device);
    bool format(CommandType *cmdtypes, CommandResponse *cmdResponse);
    bool eject(CommandType *cmdtypes, CommandResponse *cmdResponse);
    bool fsck(CommandType *cmdtypes, CommandResponse *cmdResponse);
    bool setVolumeLabel(CommandType *cmdtypes, CommandResponse *cmdResponse);
    bool umountAllDrive(CommandResponse *cmdResponse);
    bool isWritableDrive(CommandType *cmdtypes, CommandResponse *cmdResponse);
    bool mountFsck(CommandType *cmdtypes, CommandResponse *cmdResponse);
    bool getSpaceInfo (CommandType *cmdtypes, CommandResponse *cmdResponse);
    bool isStorageDevice(DeviceClass*);
	//bool isStorageDevice(PdmNetlinkEvent* pNE);
    bool umountAllDrive(bool lazyUnmount);
    void suspendRequest();
    void resumeRequest(const int &eventType);
    int readMaxUsbStorageDevices();

public:
    ~StorageDeviceHandler();
    static DeviceHandler* CreateObject(PdmConfig* const pConfObj,
                                               PluginAdapter* const pluginAdapter) {
        if (mIsObjRegistered)
            return new StorageDeviceHandler(pConfObj,pluginAdapter);
        return nullptr;
    }
    bool HandlerEvent(DeviceClass* deviceClass) override;
    //bool HandlerEvent(PdmNetlinkEvent* pNE) override;
    bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) override;
    bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) override;
    bool HandlePluginEvent(int eventType) override;
    bool GetAttachedStorageDeviceList (pbnjson::JValue &payload, LSMessage *message);
    bool GetExampleAttachedUsbStorageDeviceList (pbnjson::JValue &payload, LSMessage *message);
    void commandNotification(EventType event, Storage* device);
    void computeSpaceInfoThread();
    int getStorageDevCount() { return mStorageList.size(); }
};
#endif //_STORAGEDEVICEHANDLER_H_
