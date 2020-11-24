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

#include "MTPDevice.h"
#include "PdmLogUtils.h"
#include "PdmUtils.h"
#include <unistd.h>

using namespace PdmDevAttributes;

//MTP mount command
const std::string MTP_MOUNT_COMMAND = "simple-mtpfs -o nonempty -o auto_unmount -o allow_other ";

MTPDevice::MTPDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
            : Storage(pConfObj, pluginAdapter,"MTP",PDM_ERR_UNSUPPORT_FS,StorageInterfaceTypes::USB_MTP)
{
    fsType = PDM_DRV_TYPE_FUSE_MTP;
}

MTPDevice::~MTPDevice()
{
}

void MTPDevice::setDeviceInfo(PdmNetlinkEvent* pNE)
{
    if( pNE->getDevAttribute(DEVTYPE) == USB_DEVICE){
        if(!pNE->getDevAttribute(SPEED).empty()) {
            m_devSpeed = getDeviceSpeed(stoi(pNE->getDevAttribute(SPEED)));
        }
        driveName = pNE->getDevAttribute(DEVLINKS);
        if(!driveName.empty())
        {
            driveName.erase(0,8);
            //Example: "rootPath": "/tmp/usb/MTP-4-15"; "mountName": "/tmp/usb/MTP-4-15/MTP-4-15"
            rootPath.append(driveName);
            mountName = rootPath + "/" + driveName ;
        }
    }
    Device::setDeviceInfo(pNE);
}

PdmDevStatus MTPDevice::mtpMount( const std::string &mtpDeviceName){
    PdmDevStatus mountStatus = PdmDevStatus::PDM_DEV_UMOUNT_FAIL;

    if(!mtpDeviceName.empty() && !mountName.empty())
    {
        if(PdmUtils::createDir(mountName) && mountDevice(mtpDeviceName)){
            m_errorReason = PDM_ERR_NOTHING;
            isMounted = true;
            mountStatus = PdmDevStatus::PDM_DEV_SUCCESS;
        } else {
            PdmUtils::removeDirRecursive(rootPath);
        }
    }
    return mountStatus;
}

PdmDevStatus MTPDevice::mtpUmount(){

    PdmDevStatus umountStatus = PdmDevStatus::PDM_DEV_UMOUNT_FAIL;

    if(isMounted && unmountDevice()) {
        isMounted = false;
        umountStatus = PdmDevStatus::PDM_DEV_SUCCESS;
    }
    return umountStatus;
}

void MTPDevice::onDeviceRemove()
{
    if(!isMounted && m_errorReason != PDM_ERR_EJECTED)
        mMtpDeviceHandlerCb(REMOVE_BEFORE_MOUNT,this);

    if(m_errorReason != PDM_ERR_EJECTED)
        mtpUmount();
    PdmUtils::removeDirRecursive(rootPath);
}

PdmDevStatus MTPDevice::eject()
{
    if(mtpUmount() == PdmDevStatus::PDM_DEV_SUCCESS)
    {
        m_errorReason = PDM_ERR_EJECTED;
        return PdmDevStatus::PDM_DEV_SUCCESS;
    }
    PDM_LOG_ERROR("MTPDevice:%s line: %d MTP device eject failed" , __FUNCTION__, __LINE__);
    return PdmDevStatus::PDM_DEV_EJECT_FAIL;
}

void MTPDevice::registerCallback(handlerCb mtpDeviceHandlerCb) {
    mMtpDeviceHandlerCb = mtpDeviceHandlerCb;
}

bool MTPDevice::unmountDevice() const {

    std::string syscommand = FUSERMOUNT + mountName;
    int32_t res = system(syscommand.c_str());

    if(res) {
        PDM_LOG_ERROR("MTPDevice:%s line: %d MTP device umount failed" , __FUNCTION__, __LINE__);
        return false;
    }
    return true;
}

bool MTPDevice::mountDevice(const std::string &mtpDeviceName) {

     std::string syscommand = MTP_MOUNT_COMMAND +"/dev/"+ mtpDeviceName + " " + mountName;
     PDM_LOG_INFO("MTPDevice:",0,"%s line: %d MTP device mount CMD: %s", __FUNCTION__,__LINE__,syscommand.c_str());
     int32_t res = system(syscommand.c_str());

     if(res){
         PDM_LOG_ERROR("MTPDevice:%s line: %d MTP device mount failed res: %d, str: %s" , __FUNCTION__, __LINE__, res, strerror(errno));
         return false;
     }
     return true;
}

void MTPDevice::resumeRequest(const int &eventType) {
    m_isPowerOnConnect = true;
    setPowerStatus(true);
}
