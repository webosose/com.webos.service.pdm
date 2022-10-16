// Copyright (c) 2019-2022 LG Electronics, Inc.
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

#include "VideoDeviceHandler.h"
#include "PdmJson.h"
#include "VideoSubsystem.h"

using namespace PdmDevAttributes;

bool VideoDeviceHandler::mIsObjRegistered = VideoDeviceHandler::RegisterObject();

VideoDeviceHandler::VideoDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) :
                       DeviceHandler(pConfObj, pluginAdapter),mIsCameraReady(true),mdeviceRemoved(false){
    lunaHandler->registerLunaCallback(std::bind(&VideoDeviceHandler::GetAttachedDeviceStatus, this, _1, _2),
                                                                          GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&VideoDeviceHandler::GetAttachedNonStorageDeviceList, this, _1, _2),
                                                                                    GET_NONSTORAGEDEVICELIST);
    lunaHandler->registerLunaCallback(std::bind(&VideoDeviceHandler::GetAttachedVideoDeviceList, this, _1, _2),
                                                                                    GET_VIDEODEVICELIST);
    lunaHandler->registerLunaCallback(std::bind(&VideoDeviceHandler::GetAttachedVideoSubDeviceList, this, _1, _2),
                                                                                    GET_VIDEOSUBDEVICELIST);
}

VideoDeviceHandler::~VideoDeviceHandler() {
}

bool VideoDeviceHandler::HandlerEvent(DeviceClass* devClass)
{
    VideoSubsystem *videoSubsystem = (VideoSubsystem *)devClass;

    PDM_LOG_DEBUG("VideoDeviceHandler::HandlerEvent");
    if (devClass->getAction() == "remove")
    {
        mdeviceRemoved = false;
        ProcessVideoDevice(devClass);
        if(mdeviceRemoved) {
            PDM_LOG_DEBUG("VideoDeviceHandler:%s line: %d  DEVTYPE=usb_device removed", __FUNCTION__, __LINE__);
            return true;
        }
    }
    std::string interfaceClass = devClass->getInterfaceClass();
    if((interfaceClass.find(iClass) == std::string::npos) && (videoSubsystem->getSubsystemName() !=  "video4linux"))
        return false;
    if(devClass->getDevType() ==  USB_DEVICE) {
        ProcessVideoDevice(devClass);
        return false;
    }
    else if(devClass->getSubsystemName() ==  "video4linux") {
        ProcessVideoDevice(devClass);
        return true;
    }
    return false;
}

void VideoDeviceHandler::removeDevice(VideoDevice* videoDevice)
{
    if(!videoDevice)
        return;
    sList.remove(videoDevice);
    if(!mIsCameraReady)
        Notify(UNKNOWN_DEVICE, REMOVE, videoDevice);
    else
        Notify(VIDEO_DEVICE, REMOVE, videoDevice);
    delete videoDevice;
    videoDevice = nullptr;
}

void VideoDeviceHandler::ProcessVideoDevice(DeviceClass* devClass){
    VideoDevice *videoDevice;
    PDM_LOG_INFO("VideoDeviceHandler:",0,"%s line: %d DEVTYPE: %s SUBSYSTEM:%s ACTION: %s", __FUNCTION__,__LINE__, devClass->getDevType().c_str(), devClass->getSubsystemName().c_str(), devClass->getAction().c_str());
    try {
            switch(sMapDeviceActions.at(devClass->getAction()))
            {
                case DeviceActions::USB_DEV_ADD:
                    PDM_LOG_DEBUG("VideoDeviceHandler:%s line: %d action : %s DEVPATH: %s", __FUNCTION__, __LINE__, devClass->getAction().c_str(), devClass->getDevPath().c_str());
                    videoDevice = getDeviceWithPath< VideoDevice >(sList, devClass->getDevPath());
                    if(!videoDevice ){
                        PDM_LOG_INFO("VideoDeviceHandler",0," Created New device.");
                        videoDevice = new (std::nothrow) VideoDevice(m_pConfObj, m_pluginAdapter);
                        if(!videoDevice){
                            return;
                        }
                        videoDevice->setDeviceInfo(devClass, mIsCameraReady);
                        sList.push_back(videoDevice);
                    } else {
                        PDM_LOG_INFO("VideoDeviceHandler",0," update the video device info.");
                        videoDevice->updateDeviceInfo(devClass);
                        if(!mIsCameraReady)
                            Notify(UNKNOWN_DEVICE, ADD);
                        else {
                            if(devClass->getSubsystemName() == "video4linux" && 
								((VideoSubsystem*)devClass)->getCapabilities() == ":capture:"){
                                Notify(VIDEO_DEVICE, ADD);
                            }
                        }
                    }
                    break;
                case DeviceActions::USB_DEV_REMOVE:
                   PDM_LOG_DEBUG("VideoDeviceHandler:%s line: %d action : %s", __FUNCTION__, __LINE__, devClass->getAction().c_str());
                   videoDevice = getDeviceWithPath< VideoDevice >(sList, devClass->getDevPath());
                   if(videoDevice) {
                       removeDevice(videoDevice);
                       mdeviceRemoved = true;
                    }
                    break;
                default:
                 //Do nothing
                break;
            }
        }
        catch (const std::out_of_range& err) {
        PDM_LOG_INFO("VideoDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }
}

bool VideoDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) {

    PDM_LOG_DEBUG("VideoDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    return false;
}

bool VideoDeviceHandler::HandlePluginEvent(int eventType) {
    switch(eventType) {
    case PDM_CAM_NONE:
        mIsCameraReady = false;
        break;
    case PDM_CAM_READY:
    case PDM_CAM_BUILTIN:
        mIsCameraReady = true;
        break;
    default:
        break;
    }
    return false;
}

bool VideoDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedDeviceStatus< VideoDevice >(sList, payload );
}

bool VideoDeviceHandler::GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedNonStorageDeviceList< VideoDevice >( sList, payload );
}


bool VideoDeviceHandler::GetAttachedVideoDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
       return getAttachedVideoDeviceList< VideoDevice >( sList, payload, false);
}

bool VideoDeviceHandler::GetAttachedVideoSubDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
        return getAttachedVideoDeviceList< VideoDevice >( sList, payload, true);
}
