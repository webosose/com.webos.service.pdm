// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef STORAGEDEVICE_H_
#define STORAGEDEVICE_H_

#include <atomic>
#include <functional>
#include <list>
#include <string>
#include <thread>
#include<tuple>
#include <vector>
#include "DiskPartitionInfo.h"
#include "PdmFs.h"
#include "Storage.h"
#include "PdmIoPerf.h"

class StorageDeviceHandler;

class StorageDevice: public Storage {

private:

    bool m_deviceIsMounted;
    bool m_hasUnsupportedFs;
    bool m_isDevAddEventNotified;
    bool m_isSdCardRemoved;
    bool m_isExtraSdCard;
    int m_partitionCount;
    int  m_timeoutId;
    std::atomic<int> m_fsckThreadCount;
    std::list<DiskPartitionInfo*> m_diskPartitionList;

    using handlerCb = std::function<void(EventType, Storage*)>;
    handlerCb m_storageDeviceHandlerCb;
    PdmFs m_pdmFileSystemObj;
    std::vector<std::thread> m_fsckThreadArray;
    std::tuple <int,int,int> mHddDiskStats;

private:
   int countPartitions(const std::string &devName);
   void deletePartitionData();
   void removeRootDir();
   void updateDeviceInfo(PdmNetlinkEvent* pNE);
   void updateDiskInfo(PdmNetlinkEvent* pNE);
   void updateMultiSdCard(PdmNetlinkEvent* pNE);
   void handleCardReaderDeviceChange(PdmNetlinkEvent* pNE);
   void storageDeviceNotification();
   void fsckOnDeviceAddThread(DiskPartitionInfo *partition);
   DiskPartitionInfo* findPartition(const std::string &drivename);
   void pdmSmartDeviceInfoLogger();
   PdmDevStatus umountPartition(DiskPartitionInfo &partition, const bool lazyUnmount);
   PdmDevStatus mountPartition(DiskPartitionInfo &partition, const bool readOnly);
   PdmDevStatus fsckPartition(DiskPartitionInfo &partition, const std::string &fsckMode);
   void checkSdCardAddRemove(PdmNetlinkEvent* pNE);
   bool triggerUevent();

protected:
   int getPartitionCount() {return m_partitionCount;}
   int getdiskPartitionListSize() {return m_diskPartitionList.size();}
   void setStorageInterfaceType(PdmNetlinkEvent* pNE);

public:
   StorageDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
   ~StorageDevice();
   bool getIsMounted(){ return m_deviceIsMounted;}
   void registerCallback(handlerCb storageDeviceHandlerCb);
   void setDeviceInfo(PdmNetlinkEvent* pNE);
   PdmDevStatus setPartitionVolumeLabel(const std::string &drivename,const std::string &volumeLabel);
   void setPartitionInfo(PdmNetlinkEvent* pNE);
   PdmDevStatus umountAllPartition(const bool lazyUnmount);
   PdmDevStatus mountAllPartition();
   PdmDevStatus fsck(const std::string driveName);
   PdmDevStatus ioPerf(const std::string& driveName, unsigned int chunkSize, PdmIoPerf* perfIO);
   PdmDevStatus eject();
   PdmDevStatus isWritable(const std::string &driveName, bool &isWritable);
   bool isDevAddNotified() {return m_isDevAddEventNotified;}
   DiskPartitionInfo* getSpaceInfo(const std::string driveName, bool directCheck);
   void checkRemovalNotifications();
   PdmDevStatus formatDiskStart(const std::string driveName,std::string fsType,const std::string &volumeLabel);
   void onDeviceRemove();
   std::list<DiskPartitionInfo*> getDiskPartition(){return m_diskPartitionList;}
   void storageDeviceFsckNotification();
   bool static notifyStorageConnecting(StorageDevice *ptr);
   PdmDevStatus enforceFsckAndMount(bool needFsck);
   void suspendRequest();
   bool suspendUmountAllPartitions(const bool lazyUnmount);
   void resumeRequest(const int &eventType);
   void setIsExtraSdCard(bool value) { m_isExtraSdCard = value;}
   void setExtraSdCardDetails(IDevice &device);
   bool getIsExtraSdCard() const {return m_isExtraSdCard;}
   void setHddDiskStats(std::tuple<int,int,int> hddStats){mHddDiskStats = hddStats;}
   std::tuple<int,int,int> getHddDiskStats(){return mHddDiskStats;}
};
#endif //STORAGEDEVICE_H_
