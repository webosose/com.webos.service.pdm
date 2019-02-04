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

#include "pbnjson.hpp"
#include "PdmGetExampleUtil.h"
#include "PdmLogUtils.h"
#include "PdmLunaHandler.h"

bool PdmGetExampleUtil::printGetExampleLogs()
{
    PDM_LOG_DEBUG("PdmGetExampleUtil:%s line: %d", __FUNCTION__, __LINE__);

    PDM_LOG_DEBUG("#################################### PDM INFO ###############################################\n");

    if(!printStorageDeviceLogs())
    {
        PDM_LOG_DEBUG("PdmGetExampleUtil:%s line: %d Failed to print StorageDevice Logs", __FUNCTION__, __LINE__);
        return false;
    }
    if(!printNonStorageDeviceLogs())
    {
        PDM_LOG_DEBUG("PdmGetExampleUtil:%s line: %d Failed to print NonStorageDevice Logs", __FUNCTION__, __LINE__);
        return false;
    }
    if(!printDeviceStatusLogs())
    {
        PDM_LOG_DEBUG("PdmGetExampleUtil:%s line: %d Failed to print DeviceStatus Logs", __FUNCTION__, __LINE__);
        return false;
    }
    return true;
}


bool PdmGetExampleUtil::printStorageDeviceLogs()
{
    pbnjson::JValue payload = pbnjson::Object();
    PdmLunaHandler::getInstance()->getExampleAttachedStorageDeviceList(payload,nullptr);
    if(payload.isNull())
        return false;

    PDM_LOG_DEBUG("#################################### STORAGE DEVICE INFO ####################################\n");
    for (auto itemPair: payload.children())
    {
        if(itemPair.second.isArray()) //storageDeviceList
        {
            PDM_LOG_DEBUG("%s : \n", itemPair.first.stringify().c_str());
            int devCount = 1;
            for (auto device: itemPair.second.items())
            {
                PDM_LOG_DEBUG("************************************ StorageDevice %d Start **********************************\n", devCount);
                for (auto deviceItem: device.children())
                {
                    if(deviceItem.second.isArray()) //storageDriveList
                    {
                        PDM_LOG_DEBUG("%s : \n", deviceItem.first.stringify().c_str());
                        int driveCount = 1;
                        for (auto drive: deviceItem.second.items())
                        {
                            PDM_LOG_DEBUG("==================================== StorageDevice %d, Drive %d Start ========================\n", devCount, driveCount);
                            for (auto driveItem: drive.children())
                            {
                                PDM_LOG_DEBUG("%s : %s \n", driveItem.first.stringify().c_str(), driveItem.second.stringify().c_str());
                            }
                            PDM_LOG_DEBUG("==================================== StorageDevice %d, Drive %d End ==========================\n", devCount, driveCount);
                            driveCount++;
                        }
                        if(1 == driveCount)
                            PDM_LOG_DEBUG("Empty List\n");
                    }
                    else
                        PDM_LOG_DEBUG("%s : %s \n", deviceItem.first.stringify().c_str(), deviceItem.second.stringify().c_str());
                }
                PDM_LOG_DEBUG("************************************ StorageDevice %d End ************************************\n", devCount);
                devCount++;
            }
            if(1 == devCount)
                PDM_LOG_DEBUG("Empty List\n");
        }
        else
            PDM_LOG_DEBUG("%s : %s \n", itemPair.first.stringify().c_str(), itemPair.second.stringify().c_str());
    }
    return true;
}
bool PdmGetExampleUtil::printNonStorageDeviceLogs()
{
    pbnjson::JValue payload = pbnjson::Object();
    PdmLunaHandler::getInstance()->getAttachedNonStorageDeviceList(payload,nullptr);
    if(payload.isNull())
        return false;

    PDM_LOG_DEBUG("#################################### NONSTORAGE DEVICE INFO #################################\n");
    for (auto itemPair: payload.children())

    {
        if(itemPair.second.isArray()) //nonStorageDeviceList
        {
            PDM_LOG_DEBUG("%s : \n", itemPair.first.stringify().c_str());
            int devCount = 1;
            for (auto device: itemPair.second.items())
            {
                PDM_LOG_DEBUG("************************************ NonStorageDevice %d Start *******************************\n", devCount);
                for (auto deviceItem: device.children())
                {
                    PDM_LOG_DEBUG("%s : %s \n", deviceItem.first.stringify().c_str(), deviceItem.second.stringify().c_str());
                }
                PDM_LOG_DEBUG("************************************ NonStorageDevice %d End *********************************\n", devCount);
                devCount++;
            }
            if(1 == devCount)
                PDM_LOG_DEBUG("Empty List\n");
        }
        else
            PDM_LOG_DEBUG("%s : %s \n", itemPair.first.stringify().c_str(), itemPair.second.stringify().c_str());
    }
    return true;
}
bool PdmGetExampleUtil::printDeviceStatusLogs()
{
    pbnjson::JValue payload = pbnjson::Object();
    pbnjson::JValue deviceStatusList = pbnjson::Array();
    PdmLunaHandler::getInstance()->getAttachedDeviceStatus(deviceStatusList,nullptr);
    payload.put("deviceStatusList", deviceStatusList);
    if(payload.isNull())
        return false;

    PDM_LOG_DEBUG("#################################### DEVICE STATUS INFO #####################################\n");
    for (auto itemPair: payload.children())
    {
        if(itemPair.second.isArray()) //DeviceList
        {
            PDM_LOG_DEBUG("%s : \n", itemPair.first.stringify().c_str());
            int devCount = 1;
            for (auto device: itemPair.second.items())
            {
                PDM_LOG_DEBUG("************************************ Device %d Start *****************************************\n", devCount);
                for (auto deviceItem: device.children())
                {
                    if(deviceItem.second.isArray()) //DriveList
                    {
                        PDM_LOG_DEBUG("%s : \n", deviceItem.first.stringify().c_str());
                        int driveCount = 1;
                        for (auto drive: deviceItem.second.items())
                        {
                            PDM_LOG_DEBUG("==================================== Device %d, Drive %d Start ===============================\n", devCount, driveCount);
                            for (auto driveItem: drive.children())
                            {
                                PDM_LOG_DEBUG("%s : %s \n", driveItem.first.stringify().c_str(), driveItem.second.stringify().c_str());
                            }
                            PDM_LOG_DEBUG("==================================== Device %d, Drive %d End =================================\n", devCount, driveCount);
                            driveCount++;
                        }
                        if(1 == driveCount)
                            PDM_LOG_DEBUG("Empty List\n");
                    }
                    else
                        PDM_LOG_DEBUG("%s : %s \n", deviceItem.first.stringify().c_str(), deviceItem.second.stringify().c_str());
                }
                PDM_LOG_DEBUG("************************************ Device %d End *******************************************\n", devCount);
                devCount++;
            }
            if(1 == devCount)
                PDM_LOG_DEBUG("Empty List\n");
        }
        else
            PDM_LOG_DEBUG("%s : %s \n", itemPair.first.stringify().c_str(), itemPair.second.stringify().c_str());
    }
    return true;
}
