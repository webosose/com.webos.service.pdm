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

#include "HIDDeviceHandler.h"
#include "PdmJson.h"
#include "HIDSubsystem.h"

using namespace PdmDevAttributes;

bool HIDDeviceHandler::mIsObjRegistered = HIDDeviceHandler::RegisterObject();

HIDDeviceHandler::HIDDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) 
                 : DeviceHandler(pConfObj, pluginAdapter), m_deviceRemoved(false){
    lunaHandler->registerLunaCallback(std::bind(&HIDDeviceHandler::GetAttachedDeviceStatus, this, _1, _2),
                                                                          GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&HIDDeviceHandler::GetAttachedNonStorageDeviceList, this, _1, _2),
                                                                                    GET_NONSTORAGEDEVICELIST);
}

HIDDeviceHandler::~HIDDeviceHandler() {
}

bool HIDDeviceHandler::HandlerEvent(DeviceClass* devClass)
{
    HIDSubsystem *hidSubsystem = (HIDSubsystem *)devClass;
    if (hidSubsystem == nullptr) return false;

    PDM_LOG_DEBUG("HIDDeviceHandler::HandlerEvent");
    PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    if (hidSubsystem->getAction()== "remove")
    {
        m_deviceRemoved = false;
        ProcessHIDDevice(hidSubsystem);
        if(m_deviceRemoved) {
            PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d  DEVTYPE=usb_device removed", __FUNCTION__, __LINE__);
            return true;
        }
    }

    std::string interfaceClass = hidSubsystem->getInterfaceClass();
    if(interfaceClass.find(iClass) == std::string::npos)
        return false;
    if(hidSubsystem->getDevType()==  USB_DEVICE){
        ProcessHIDDevice(hidSubsystem);
        return false;
    }
    else if(hidSubsystem->getSubsystemName()==  "input") {
        ProcessHIDDevice(hidSubsystem);
        return true;
    }
    return false;
}

#if 0
bool HIDDeviceHandler::HandlerEvent(PdmNetlinkEvent* pNE){

    PDM_LOG_DEBUG("HIDDeviceHandler::HandlerEvent");
    if (pNE->getDevAttribute(ACTION) == "remove")
    {
        m_deviceRemoved = false;
        ProcessHIDDevice(pNE);
        if(m_deviceRemoved) {
            PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d  DEVTYPE=usb_device removed", __FUNCTION__, __LINE__);
            return true;
        }
    }

    std::string interfaceClass = pNE->getInterfaceClass();
    if(interfaceClass.find(iClass) == std::string::npos)
        return false;
    if(pNE->getDevAttribute(DEVTYPE) ==  USB_DEVICE){
        ProcessHIDDevice(pNE);
        return false;
    }
    else if(pNE->getDevAttribute(SUBSYSTEM) ==  "input") {
        ProcessHIDDevice(pNE);
        return true;
    }
    return false;
}
#endif

void HIDDeviceHandler::removeDevice(HIDDevice* hidDevice)
{
PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d removing HID: %s",__FUNCTION__, __LINE__, hidDevice->getDevicePath().c_str());
    if(!hidDevice)
        return;
    sList.remove(hidDevice);
PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d",__FUNCTION__, __LINE__);
    Notify(HID_DEVICE,REMOVE,hidDevice);
PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d",__FUNCTION__, __LINE__);
    delete hidDevice;
    hidDevice = nullptr;

}

void HIDDeviceHandler::ProcessHIDDevice(DeviceClass* devClass){
    HIDDevice *hidDevice;
    PDM_LOG_INFO("HIDDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__, __LINE__, devClass->getDevType().c_str(), devClass->getAction().c_str());
    PDM_LOG_DEBUG("HIDDeviceHandler: %s line: %d DEVPATH: %s", __FUNCTION__, __LINE__, devClass->getDevPath().c_str());

   try {
        PDM_LOG_DEBUG("HIDDeviceHandler: %s line: %d ", __FUNCTION__, __LINE__);
            switch(sMapDeviceActions.at(devClass->getAction()))
            {
                PDM_LOG_DEBUG("HIDDeviceHandler: %s line: %d ", __FUNCTION__, __LINE__);
                case DeviceActions::USB_DEV_ADD:
                PDM_LOG_DEBUG("HIDDeviceHandler: %s line: %d ", __FUNCTION__, __LINE__);
                    hidDevice = getDeviceWithPath< HIDDevice >(sList, devClass->getDevPath());
                    // hidDevice = getDeviceWithPath< HIDDevice >(sList, m_devicePath);
                    
                    if(!hidDevice)
                    {
                        PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
                        hidDevice = new (std::nothrow) HIDDevice(m_pConfObj, m_pluginAdapter);
                        if(!hidDevice)
                            break;
                        hidDevice->setDeviceInfo(devClass);
                        PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
                        hidDevice->registerCallback(std::bind(&HIDDeviceHandler::commandNotification, this, _1, _2));
                        sList.push_back(hidDevice);
                        for(auto l : sList) {
                            PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d  SoundDveicePath: %s",__FUNCTION__, __LINE__, l->getDevicePath().c_str());
                        }
                    }else {
                        PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
                        hidDevice->setDeviceInfo(devClass);
                    }
                    for(auto l : sList) {
                            PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d  SoundDveicePath: %s",__FUNCTION__, __LINE__, l->getDevicePath().c_str());
                        }
                    break;
                case DeviceActions::USB_DEV_REMOVE:
                PDM_LOG_DEBUG("HIDDeviceHandler: %s line: %d ", __FUNCTION__, __LINE__);
                    for(auto l : sList) {
                        PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d  SoundDveicePath: %s",__FUNCTION__, __LINE__, l->getDevicePath().c_str());
                    }
                    hidDevice = getDeviceWithPath< HIDDevice >(sList, devClass->getDevPath());
                    if(hidDevice) {
                       removeDevice(hidDevice);
                       m_deviceRemoved = true;
                    }
                    PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d ",__FUNCTION__, __LINE__);
                    for(auto l : sList) {
                        PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d  SoundDveicePath: %s",__FUNCTION__, __LINE__, l->getDevicePath().c_str());
                    }
                    break;
                default:
                    PDM_LOG_DEBUG("HIDDeviceHandler: %s line: %d ACTION NOT found", __FUNCTION__, __LINE__);
                //Do nothing
                   break;
            }
       }
      catch (const std::out_of_range& err) {
         PDM_LOG_INFO("HIDDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__, __LINE__, err.what());
   }
}

#if 0
void HIDDeviceHandler::ProcessHIDDevice(PdmNetlinkEvent* pNE){
    HIDDevice *hidDevice;
    PDM_LOG_INFO("HIDDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__,__LINE__,pNE->getDevAttribute(DEVTYPE).c_str(),pNE->getDevAttribute(ACTION).c_str());

   try {
            switch(sMapDeviceActions.at(pNE->getDevAttribute(ACTION)))
            {
                case DeviceActions::USB_DEV_ADD:
                    hidDevice = getDeviceWithPath< HIDDevice >(sList,pNE->getDevAttribute(DEVPATH));
                    if(!hidDevice)
                    {
                        hidDevice = new (std::nothrow) HIDDevice(m_pConfObj, m_pluginAdapter);
                        if(!hidDevice)
                            break;
                        hidDevice->setDeviceInfo(pNE);
                        hidDevice->registerCallback(std::bind(&HIDDeviceHandler::commandNotification, this, _1, _2));
                        sList.push_back(hidDevice);
                    }else
                        hidDevice->setDeviceInfo(pNE);
                    break;
                case DeviceActions::USB_DEV_REMOVE:
                    hidDevice = getDeviceWithPath< HIDDevice >(sList,pNE->getDevAttribute(DEVPATH));
                    if(hidDevice) {
                       removeDevice(hidDevice);
                       m_deviceRemoved = true;
                    }
                    break;
                default:
                //Do nothing
                   break;
            }
       }
      catch (const std::out_of_range& err) {
         PDM_LOG_INFO("HIDDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
   }
}
#endif

bool HIDDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) {

    PDM_LOG_DEBUG("HIDDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    return false;
}

bool HIDDeviceHandler::HandlePluginEvent(int eventType)
{
    PDM_LOG_DEBUG("HIDDeviceHandler: %s line: %d Event Type %d", __FUNCTION__, __LINE__, eventType);
    return false;
}

bool HIDDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedDeviceStatus< HIDDevice >(sList, payload );
}

bool HIDDeviceHandler::GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedNonStorageDeviceList< HIDDevice >( sList, payload );
}

void HIDDeviceHandler::commandNotification(EventType event, HIDDevice* device)
{
    Notify(HID_DEVICE,event,device);
}
