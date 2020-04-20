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

#ifndef STORAGE_H
#define STORAGE_H

#include <string>

#include "Device.h"

enum StorageInterfaceTypes {USB_STICK = 0, USB_HDD, USB_CARD_READER, USB_MTP, USB_PTP, EMMC, USB_UNDEFINED};

// Map to associate storage type with the StorageType enum values
static std::map<StorageInterfaceTypes, std::string> sMapStorageType= {
    {USB_STICK,            "FLASH"},
    {USB_HDD,              "HDD"},
    {USB_CARD_READER,      "FLASH"},
    {USB_MTP,              "STREAMING"},
    {USB_PTP,              "STREAMING"},
    {EMMC,                 "EMMC"},
    {USB_UNDEFINED,        "UNKNOWN"}
};

// This class is for storage specific varibles and functions
class Storage : public Device
{
private:
    std::string readRootPath();

protected:
    bool isMounted;
    bool isReadOnly;
    bool powerStatus;
    bool isDirCreated;
    uint64_t    m_driveSize;
    uint64_t    m_usedSize;
    uint64_t    m_freeSize;
    uint64_t    m_usedRate;
    std::string volumeLabel;
    std::string uuid;
    std::string driveName;
    std::string fsType;
    std::string mountName;
    std::string rootPath;
    StorageInterfaceTypes m_storageType;

public:
    Storage(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter, std::string deviceType
                      , std::string errorReason, StorageInterfaceTypes storageType);
    virtual ~Storage()= default;
    virtual bool getIsMounted(){ return isMounted;}
    virtual const std::string getVolumeLable() { return volumeLabel;}
    virtual const std::string getUuid() { return uuid;}
    virtual const std::string getDriveName(){ return driveName;}
    virtual const std::string getFsType(){ return fsType;}
    virtual const std::string getMountName(){ return mountName;}
    virtual StorageInterfaceTypes getStorageType(){return m_storageType;}
    virtual const std::string getStorageTypeString(){return sMapStorageType[m_storageType];}
    virtual void setPowerStatus(const bool &status) {powerStatus = status;}
    virtual bool getPowerStatus() const {return powerStatus;}
    std::string getRootPath(){return rootPath;}
    void setStorageType(StorageInterfaceTypes type){m_storageType = type;}
    void setDriveSize(unsigned long driveSize){ m_driveSize = driveSize; }
    unsigned long getDriveSize() { return m_driveSize;}
    void setUsedSize(unsigned long usedSize) { m_usedSize = usedSize; }
    unsigned long getUsedSize() { return m_usedSize;}
    void setFreeSize(unsigned long freeSize) { m_freeSize = freeSize; }
    unsigned long getFreeSize() { return m_freeSize;}
    void setUsedRate(unsigned long usedRate) { m_usedRate = usedRate; }
    unsigned long getUsedRate() { return m_usedRate;}
};
#endif //STORAGE_H
