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

#ifndef _PDM_JSON_H
#define _PDM_JSON_H

#include <list>
#include "PdmLunaHandler.h"
#include "CdcDevice.h"

template < class T > bool getAttachedDeviceStatus(std::list<T*>& sList, pbnjson::JValue &payload)
{
    if(sList.empty())
        return false;
    for( auto device : sList)
    {
        pbnjson::JValue deviceStatusListObj = pbnjson::Object();
        deviceStatusListObj.put("deviceNum", (int32_t)device->getDeviceNum());
        deviceStatusListObj.put("deviceStatus", device->getDeviceStatus());
        payload.append(deviceStatusListObj);
    }
    return true;
}

template < class T > bool getAttachedStreamingDeviceStatus(const std::list<T*>& sList, pbnjson::JValue &payload)
{
    if(sList.empty())
        return false;
    for( auto device : sList)
    {
        pbnjson::JValue deviceStatusListObj = pbnjson::Object();
        deviceStatusListObj.put("deviceNum", (int32_t)device->getDeviceNum());
        deviceStatusListObj.put("deviceStatus", device->getDeviceStatus());
        pbnjson::JValue driveStatusList = pbnjson::Array();
        pbnjson::JValue driveStatus = pbnjson::Object();
        driveStatus.put("driveName", device->getDriveName());
        driveStatus.put("driveStatus",device->getDriveStatus());
        driveStatusList.append(driveStatus);
        deviceStatusListObj.put("driveStatusList", driveStatusList);
        payload.append(deviceStatusListObj);
    }
    return true;
}

template < class T > bool getAttachedStorageDeviceStatus(std::list<T*>& sList, pbnjson::JValue &payload)
{
    if(sList.empty())
        return false;
    for( auto device : sList)
    {
        pbnjson::JValue deviceStatusListObj = pbnjson::Object();
        deviceStatusListObj.put("deviceNum", (int32_t)device->getDeviceNum());
        deviceStatusListObj.put("deviceStatus", device->getDeviceStatus());
        if(device->getDeviceType() == "USB_STORAGE")
        {
            pbnjson::JValue driveStatusList = pbnjson::Array();
            for(auto disk : device->getDiskPartition())
            {
                pbnjson::JValue driveStatus = pbnjson::Object();
                driveStatus.put("driveName", disk->getDriveName());
                driveStatus.put("driveStatus", disk->getDriveStatus());
                driveStatusList.append(driveStatus);
            }
            deviceStatusListObj.put("driveStatusList", driveStatusList);
        }
        payload.append(deviceStatusListObj);
    }
    return true;
}

template < class T > bool getAttachedNonStorageDeviceList(std::list<T*>& sList, pbnjson::JValue &payload)
{
   if(sList.empty())
        return false;

    for(auto device: sList)
    {
#ifdef WEBOS_SESSION
       device->setDeviceSetId(device->getHubPortNumber());
       CdcDevice *cdcDevice = reinterpret_cast<CdcDevice*> (device);
       if((cdcDevice && ("USB2SERIAL" == cdcDevice->getDeviceType()) /*&& (cdcDevice->getUsb2SerialProcessStatus() == false)*/) || (device ->getProductName().find("eGalaxTouch") != std::string::npos) || (device->getDeviceType().find("SOUND") != std::string::npos))
       {
            continue;
       }
       pbnjson::JValue nonStorageDeviceObj = pbnjson::Object();
       nonStorageDeviceObj.put("deviceNum",(int64_t)device->getDeviceNum());
       nonStorageDeviceObj.put("usbPortNum", (int64_t)device->getUsbPortNumber());
       nonStorageDeviceObj.put("vendorName", device->getVendorName());
       nonStorageDeviceObj.put("productName", device->getProductName());
       nonStorageDeviceObj.put("serialNumber", device->getSerialNumber());
       nonStorageDeviceObj.put("deviceType", device->getDeviceType());
       nonStorageDeviceObj.put("deviceSubtype", device->getDeviceSubType());
       nonStorageDeviceObj.put("isPowerOnConnect", device->isConnectedToPower());
       nonStorageDeviceObj.put("devSpeed", device->getDevSpeed());
       nonStorageDeviceObj.put("devPath", device->getDevPath());
       nonStorageDeviceObj.put("hubPortPath", device->getHubPortNumber());
       nonStorageDeviceObj.put("vendorId", device->getVendorID());
       nonStorageDeviceObj.put("productId", device->getProductID());
       if(device->getDeviceType() == "BLUETOOTH") {
            nonStorageDeviceObj.put("deviceName", device->getDeviceName());
       }
       nonStorageDeviceObj.put("deviceSetId", device->getDeviceSetId());
#else
       pbnjson::JValue nonStorageDeviceObj = pbnjson::Object();
       nonStorageDeviceObj.put("deviceNum",(int64_t)device->getDeviceNum());
       nonStorageDeviceObj.put("usbPortNum", (int64_t)device->getUsbPortNumber());
       nonStorageDeviceObj.put("vendorName", device->getVendorName());
       nonStorageDeviceObj.put("productName", device->getProductName());
       nonStorageDeviceObj.put("serialNumber", device->getSerialNumber());
       nonStorageDeviceObj.put("deviceType", device->getDeviceType());
       nonStorageDeviceObj.put("deviceSubtype", device->getDeviceSubType());
       nonStorageDeviceObj.put("isPowerOnConnect", device->isConnectedToPower());
       nonStorageDeviceObj.put("devSpeed", device->getDevSpeed());
#endif
       payload.append(nonStorageDeviceObj);
   }
   return true;
}
template < class T > bool getAttachedStorageDeviceList (std::list<T*>& sList, pbnjson::JValue &payload)
{
    if(sList.empty())
        return false;

    for( auto storageIter : sList )
    {
#ifdef WEBOS_SESSION
        if(storageIter->getErrorReason() == "NOMOUNTED")
            continue;

        pbnjson::JValue storageDevice = pbnjson::Object();
        pbnjson::JValue storageDriveList = pbnjson::Array();
        storageDevice.put("deviceNum", (int32_t)storageIter->getDeviceNum());
        storageDevice.put("usbPortNum", (int32_t)storageIter->getUsbPortNumber());
        storageDevice.put("vendorName",  storageIter->getVendorName());
        storageDevice.put("productName", storageIter->getProductName());
        storageDevice.put("serialNumber", storageIter->getSerialNumber());
        storageDevice.put("deviceType", storageIter->getDeviceType());
        storageDevice.put("storageType", storageIter->getStorageTypeString());
        storageDevice.put("rootPath", storageIter->getRootPath());
        storageDevice.put("isPowerOnConnect", storageIter->isConnectedToPower());
        storageDevice.put("devSpeed", storageIter->getDevSpeed());
        storageDevice.put("errorReason", storageIter->getErrorReason());
        //storageDevice.put("hubPortPath", storageIter->getHubPortNumber());
        pbnjson::JValue driveInfo = pbnjson::Object();
        driveInfo.put("driveName", storageIter->getDriveName());
        driveInfo.put("mountName", storageIter->getMountName());
        driveInfo.put("uuid", storageIter->getUuid());
        driveInfo.put("volumeLabel", storageIter->getVolumeLable());
        driveInfo.put("fsType", storageIter->getFsType());
        driveInfo.put("driveSize", (int64_t) storageIter->getDriveSize());

        if(storageIter->getPowerStatus())
            driveInfo.put("isMounted", storageIter->getIsMounted());
        else  //in suspend case before umount need to send isMounted as false
            driveInfo.put("isMounted", false);

        storageDriveList.append(driveInfo);
        storageDevice.put("storageDriveList", storageDriveList);
#else
        pbnjson::JValue storageDevice = pbnjson::Object();
        pbnjson::JValue storageDriveList = pbnjson::Array();
        storageDevice.put("deviceNum", (int32_t)storageIter->getDeviceNum());
        storageDevice.put("usbPortNum", (int32_t)storageIter->getUsbPortNumber());
        storageDevice.put("vendorName",  storageIter->getVendorName());
        storageDevice.put("productName", storageIter->getProductName());
        storageDevice.put("serialNumber", storageIter->getSerialNumber());
        storageDevice.put("deviceType", storageIter->getDeviceType());
        storageDevice.put("storageType", storageIter->getStorageTypeString());
        storageDevice.put("rootPath", storageIter->getRootPath());
        storageDevice.put("isPowerOnConnect", storageIter->isConnectedToPower());
        storageDevice.put("devSpeed", storageIter->getDevSpeed());
        storageDevice.put("errorReason", storageIter->getErrorReason());
        pbnjson::JValue driveInfo = pbnjson::Object();
        driveInfo.put("driveName", storageIter->getDriveName());
        driveInfo.put("mountName", storageIter->getMountName());
        driveInfo.put("uuid", storageIter->getUuid());
        driveInfo.put("volumeLabel", storageIter->getVolumeLable());
        driveInfo.put("fsType", storageIter->getFsType());
        driveInfo.put("driveSize", (int32_t)storageIter->getDriveSize());

        if(storageIter->getPowerStatus())
            driveInfo.put("isMounted", storageIter->getIsMounted());
        else  //in suspend case before umount need to send isMounted as false
            driveInfo.put("isMounted", false);

        storageDriveList.append(driveInfo);
        storageDevice.put("storageDriveList", storageDriveList);
#endif
        payload.append(storageDevice);
    }
    return true;
}

template < class T > bool getAttachedUsbStorageDeviceList (std::list<T*>& sList, pbnjson::JValue &payload)
{
    if(sList.empty())
        return false;

    for( auto storageIter : sList )
    {
#ifdef WEBOS_SESSION
        storageIter->setDeviceSetId(storageIter->getHubPortNumber());
        if((storageIter->getDiskPartition().empty())|| (storageIter->getErrorReason() == "NOMOUNTED") || (storageIter->getErrorReason() == "USB30_BLACKDEVICE") || (storageIter->getErrorReason() == "UNSUPPORT_FILESYSTEM"))
#else
        if(storageIter->getDiskPartition().empty())
#endif
            continue;
        pbnjson::JValue storageDevice = pbnjson::Object();
        pbnjson::JValue storageDriveList = pbnjson::Array();
        for(auto disk : storageIter->getDiskPartition())
        {
            pbnjson::JValue driveInfo = pbnjson::Object();
#ifndef WEBOS_SESSION
            if(disk->getPowerStatus())
                driveInfo.put("isMounted", disk->isMounted());
            else //in suspend case before umount need to send isMounted as false
            driveInfo.put("isMounted", false);
            driveInfo.put("mountName", disk->getMountName());
            driveInfo.put("driveSize", (int32_t)disk->getDriveSize());
#else
            driveInfo.put("isMounted", disk->isPartitionMounted(storageIter->getHubPortNumber()));
            driveInfo.put("mountName", disk->getPartitionMountName(storageIter->getHubPortNumber(), disk->getDriveName()));
            driveInfo.put("driveSize", disk->getPartitionSize(storageIter->getHubPortNumber(), disk->getDriveName()));
#endif
            driveInfo.put("volumeLabel", disk->getVolumeLable());
            driveInfo.put("uuid", disk->getUuid());
            driveInfo.put("driveName", disk->getDriveName());
            driveInfo.put("fsType", disk->getFsType());

            storageDriveList.append(driveInfo);
        }
        storageDevice.put("storageDriveList", storageDriveList);
        storageDevice.put("deviceNum", (int32_t)storageIter->getDeviceNum());
        storageDevice.put("usbPortNum",(int32_t)storageIter->getUsbPortNumber());
        storageDevice.put("vendorName",  storageIter->getVendorName());
        storageDevice.put("productName", storageIter->getProductName());
        storageDevice.put("serialNumber",storageIter->getSerialNumber());
        storageDevice.put("deviceType", storageIter->getDeviceType());
        storageDevice.put("storageType", storageIter->getStorageTypeString());
#ifdef WEBOS_SESSION
        storageDevice.put("hubPortPath", storageIter->getHubPortNumber());
        storageDevice.put("errorReason", storageIter->getErrorReason(storageIter->getHubPortNumber()));
        storageDevice.put("vendorId", storageIter->getVendorID());
        storageDevice.put("productId", storageIter->getProductID());
        storageDevice.put("deviceSetId", storageIter->getDeviceSetId());
        storageDevice.put("rootPath", storageIter->getStorageRootPath(storageIter->getDeviceSetId()));
#else
        storageDevice.put("rootPath", storageIter->getRootPath());
        storageDevice.put("errorReason", storageIter->getErrorReason());
#endif
        storageDevice.put("isPowerOnConnect", storageIter->isConnectedToPower());
        storageDevice.put("devSpeed", storageIter->getDevSpeed());
        payload.append(storageDevice);
    }
    return true;
}

template < class T > bool getExampleAttachedUsbStorageDeviceList (std::list<T*>& sList, pbnjson::JValue &payload)
{
    if(sList.empty())
        return false;

    for( auto storageIter : sList )
    {
        if(storageIter->getDiskPartition().empty())
            continue;
        pbnjson::JValue storageDevice = pbnjson::Object();
        pbnjson::JValue storageDriveList = pbnjson::Array();
        for(auto disk : storageIter->getDiskPartition())
        {
            pbnjson::JValue driveInfo = pbnjson::Object();

            if(disk->getPowerStatus())
                driveInfo.put("isMounted", disk->isMounted());
            else //in suspend case before umount need to send isMounted as false
                driveInfo.put("isMounted", false);

            auto diskInfo = storageIter->getSpaceInfo(disk->getDriveName(),true);
            if(diskInfo){
                if(diskInfo->isMounted()) {
                    pbnjson::JValue detailedSpaceInfo = pbnjson::Object();
                    detailedSpaceInfo.put("totalSize",(int32_t) diskInfo->getDriveSize());
                    detailedSpaceInfo.put("freeSize", (int32_t) diskInfo->getFreeSize());
                    detailedSpaceInfo.put("usedSize", (int32_t) diskInfo->getUsedSize());
                    detailedSpaceInfo.put("usedRate", (int32_t) diskInfo->getUsedRate());
                    driveInfo.put("spaceInfo",detailedSpaceInfo);
                }
            }

            driveInfo.put("volumeLabel", disk->getVolumeLable());
            driveInfo.put("uuid", disk->getUuid());
            driveInfo.put("driveName", disk->getDriveName());
            driveInfo.put("driveSize", (int32_t)disk->getDriveSize());
            driveInfo.put("fsType", disk->getFsType());
            driveInfo.put("mountName", disk->getMountName());

            storageDriveList.append(driveInfo);
        }
        storageDevice.put("storageDriveList", storageDriveList);
        storageDevice.put("deviceNum", (int32_t)storageIter->getDeviceNum());
        storageDevice.put("usbPortNum",(int32_t)storageIter->getUsbPortNumber());
        storageDevice.put("vendorName",  storageIter->getVendorName());
        storageDevice.put("productName", storageIter->getProductName());
        storageDevice.put("serialNumber",storageIter->getSerialNumber());
        storageDevice.put("deviceType", storageIter->getDeviceType());
        storageDevice.put("storageType", storageIter->getStorageTypeString());
        storageDevice.put("rootPath", storageIter->getRootPath());
        storageDevice.put("isPowerOnConnect", storageIter->isConnectedToPower());
        storageDevice.put("devSpeed", storageIter->getDevSpeed());
        storageDevice.put("errorReason", storageIter->getErrorReason());
        payload.append(storageDevice);
    }
    return true;
}

template < class T > bool getAttachedAudioDeviceList(std::list<T*>& sList, pbnjson::JValue &payload)
{
   if(sList.empty())
        return false;

    for(auto device: sList)
    {
       pbnjson::JValue audioDeviceObj = pbnjson::Object();
       audioDeviceObj.put("cardName", device->getCardName());
       audioDeviceObj.put("cardId", device->getCardId());
       audioDeviceObj.put("cardNumber", device->getCardNumber());
       audioDeviceObj.put("usbPortNum", (int64_t)device->getUsbPortNumber());
       audioDeviceObj.put("devSpeed", device->getDevSpeed());
       payload.append(audioDeviceObj);
   }
   return true;
}

template < class T > bool getAttachedVideoDeviceList(std::list<T*>& sList, pbnjson::JValue &payload)
{
   if(sList.empty())
        return false;

    for(auto device: sList)
    {
       pbnjson::JValue videoDeviceObj = pbnjson::Object();
       videoDeviceObj.put("KERNEL", device->getKernel());
       videoDeviceObj.put("SUBSYSTEM", device->getSubsystem());
       videoDeviceObj.put("vendorName", device->getVendorName());
       videoDeviceObj.put("productName", device->getProductName());
       videoDeviceObj.put("devSpeed", device->getDevSpeed());
       payload.append(videoDeviceObj);
    }
   return true;
}

template < class T > bool getAttachedNetDeviceList(std::list<T*>& sList, pbnjson::JValue &payload)
{
   if(sList.empty())
        return false;

    for(auto device: sList)
    {
       if(device->getDeviceType() == "CDC"){
        pbnjson::JValue netDeviceObj = pbnjson::Object();
        netDeviceObj.put("operstate", device->getOperstate());
        netDeviceObj.put("ifindex", device->getDeviceifindex());
        netDeviceObj.put("linkmode", device->getDeviceLinkMode());
        netDeviceObj.put("duplex", device->getDuplex());
        netDeviceObj.put("address", device->getDeviceAddress());
        netDeviceObj.put("vendorName", device->getVendorName());
        netDeviceObj.put("productName", device->getProductName());
        netDeviceObj.put("devSpeed", device->getDevSpeed());
        payload.append(netDeviceObj);
       }
    }
   return true;
}
#endif //_PDM_JSON_H
