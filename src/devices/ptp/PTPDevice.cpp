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

#include <sstream>
#include <iomanip>
#include <dirent.h>
#include "PdmLogUtils.h"
#include "PdmUtils.h"
#include "PTPDevice.h"

using namespace PdmDevAttributes;

PTPDevice::PTPDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
            : Storage(pConfObj, pluginAdapter, "PTP",PDM_ERR_UNSUPPORT_FS,StorageInterfaceTypes::USB_PTP)
            , m_ptpDevNum(0)
            , m_driveStatus("") {
    fsType = PDM_DRV_TYPE_FUSE_PTP;
}

PdmDevStatus PTPDevice::ptpMount() {

    PdmDevStatus mountStatus = PdmDevStatus::PDM_DEV_ROOT_PATH_EMPTY;
    if(rootPath.empty()) {
        PDM_LOG_ERROR("PTPDevice:%s line: %d PTP device mount failed, rootPath empty" , __FUNCTION__, __LINE__);
        return mountStatus;
    }
    std::string PTPDirName = "PTP-" + std::to_string(m_busNum) + "-" + std::to_string(m_ptpDevNum);
    rootPath += PTPDirName;
    mountName = rootPath + "/" + PTPDirName;
    driveName = PTPDirName;

    if (PdmUtils::createDir(mountName) && mountDevice()) {
        isMounted = true;
        m_errorReason = PDM_ERR_NOTHING;
        mountStatus = PdmDevStatus::PDM_DEV_SUCCESS;
        m_driveStatus = MOUNT_OK;
    } else {
        mountStatus = PdmDevStatus::PDM_DEV_UMOUNT_FAIL;
        m_driveStatus = MOUNT_NOT_OK;
    }

    return mountStatus;
}

PdmDevStatus PTPDevice::ptpUmount() {

    PdmDevStatus umountStatus = PdmDevStatus::PDM_DEV_UMOUNT_FAIL;
    m_driveStatus = UMOUNT_NOT_OK;

    if(isMounted && unmountDevice()){
        isMounted = false;
        umountStatus = PdmDevStatus::PDM_DEV_SUCCESS;
        m_driveStatus = UMOUNT_OK;
    }
    return umountStatus;
}

void PTPDevice::onDeviceRemove()
{
    if(m_errorReason != PDM_ERR_EJECTED)
        ptpUmount();
    PdmUtils::removeDirRecursive(rootPath);
}

void PTPDevice::setDeviceInfo(DeviceClass* devClass)
{
    if (devClass->getDevType() == USB_DEVICE) {
        if(!devClass->getSpeed().empty()) {
            m_devSpeed = getDeviceSpeed(stoi(devClass->getSpeed()));
        }
        if(!devClass->getBusNum().empty()) {
            m_busNum = std::stoi(devClass->getBusNum());
        }
        if(!devClass->getDevNumber().empty()) {
            m_ptpDevNum = std::stoi(devClass->getDevNumber(), nullptr);
        }
        Device::setDeviceInfo(devClass);
    }
    if(m_deviceNum != 0 && m_busNum !=0){
        ptpMount();
    }
}

#if 0
void PTPDevice::setDeviceInfo(PdmNetlinkEvent* pNE)
{
    if (pNE->getDevAttribute(DEVTYPE) == USB_DEVICE) {
        if(!pNE->getDevAttribute(SPEED).empty()) {
            m_devSpeed = getDeviceSpeed(stoi(pNE->getDevAttribute(SPEED)));
        }
        if(!pNE->getDevAttribute(BUSNUM).empty()) {
            m_busNum = std::stoi(pNE->getDevAttribute(BUSNUM));
        }
        if(!pNE->getDevAttribute(DEVNUM).empty()) {
            m_ptpDevNum = std::stoi(pNE->getDevAttribute(DEVNUM),nullptr);
        }
        Device::setDeviceInfo(pNE);
    }
    if(m_deviceNum != 0 && m_busNum !=0){
        ptpMount();
    }
}
#endif

PdmDevStatus PTPDevice::eject()
{
    if(ptpUmount() == PdmDevStatus::PDM_DEV_SUCCESS) {
        m_errorReason = PDM_ERR_EJECTED;
        return PdmDevStatus::PDM_DEV_SUCCESS;
    }
    return PdmDevStatus::PDM_DEV_EJECT_FAIL;
}

bool PTPDevice::mountDevice() {
    std::stringstream sysCommandStream;
    sysCommandStream << PTP_MOUNT_COMMAND << " --port=usb:" << std::setfill('0')
                     << std::setw(3) << m_busNum << "," << std::setfill('0')
                     << std::setw(3) << m_ptpDevNum << " " <<  mountName;

    int res = system(sysCommandStream.str().c_str());
    if(res){
        PDM_LOG_ERROR("PTPDevice:%s line: %d PTP device mount failed %d " , __FUNCTION__, __LINE__, res);
        return false;
    }

    if(checkDirectory(mountName)){
       return true;
    }
    PDM_LOG_ERROR("PTPDevice:%s line: %d PTP device mount empty %d " , __FUNCTION__, __LINE__, res);
    return false;
}

bool PTPDevice::unmountDevice() const {

    if(!isMounted)
        return true;

    std::string sysCommand = FUSERMOUNT + mountName;
    int32_t res = system(sysCommand.c_str());

    if(res) {
        PDM_LOG_ERROR("PTPDevice:%s line: %d PTP device umount failed" , __FUNCTION__, __LINE__);
        return false;
    }
    return true;
}

void PTPDevice::resumeRequest(const int &eventType) {

    m_isPowerOnConnect = true;
    setPowerStatus(true);
}

void PTPDevice::registerCallback(handlerCb ptpDeviceHandlerCb) {
    mPtpDeviceHandlerCb = ptpDeviceHandlerCb;
}


bool PTPDevice::checkDirectory(const std::string &path) {
    bool retValue = false;
    struct dirent   *pDirEnt;

    if(path.empty())
        return retValue;

    DIR *pDir = opendir(path.c_str());
    if (!pDir) {
        PDM_LOG_ERROR("PTPDevice:%s line: %d not able to open %s" , __FUNCTION__, __LINE__, path.c_str());
        return retValue;
    }

    while((pDirEnt = readdir(pDir))!= nullptr) {

        std::string dirName = pDirEnt->d_name;

        if( dirName.compare(".")  &&  dirName.compare("..")) {
            std::string subPath = path + "/"+ dirName;
            retValue = checkEmpty(subPath);
            PDM_LOG_DEBUG("PTPDevice:%s line: %d checkEmpty name %s  :  %d " , __FUNCTION__, __LINE__,dirName.c_str() , retValue);
            break;
       }
    }
    closedir(pDir);
    return retValue;
}

bool PTPDevice::checkEmpty(const std::string &path) {
    bool retValue = false;
    struct dirent   *pDirEnt;

    if(path.empty()) {
        PDM_LOG_ERROR("PTPDevice:%s line: %d not able to open directory : [%s]" , __FUNCTION__, __LINE__, path.c_str());
        return retValue;
    }

    DIR *pDir = opendir(path.c_str());
    if (!pDir) {
        PDM_LOG_ERROR("PTPDevice:%s line: %d not able to open directory : [%s]" , __FUNCTION__, __LINE__, path.c_str());
        return retValue;
    }

    while((pDirEnt = readdir(pDir)) != nullptr) {
        std::string dirName = pDirEnt->d_name;
         PDM_LOG_DEBUG("PTPDevice:%s line: %d dic name : [%s]" , __FUNCTION__, __LINE__,dirName.c_str());
         if( dirName.compare(".")  &&  dirName.compare("..")) {
            PDM_LOG_DEBUG("PTPDevice:%s line: %d dic name : [%s] : %d" , __FUNCTION__, __LINE__,dirName.c_str() , retValue);
            retValue = true;
            break;
        }
    }
    closedir(pDir);
    return retValue;
}
