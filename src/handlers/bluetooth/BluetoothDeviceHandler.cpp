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

#include "BluetoothDeviceHandler.h"
#include "PdmJson.h"

using namespace PdmDevAttributes;

bool BluetoothDeviceHandler::mIsObjRegistered = BluetoothDeviceHandler::RegisterObject();
BluetoothDeviceHandler::BluetoothDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) : DeviceHandler(pConfObj, pluginAdapter) {
    lunaHandler->registerLunaCallback(std::bind(&BluetoothDeviceHandler::GetAttachedDeviceStatus, this, _1, _2),
                                      GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&BluetoothDeviceHandler::GetAttachedNonStorageDeviceList, this, _1, _2),
                                      GET_NONSTORAGEDEVICELIST);
}
BluetoothDeviceHandler::~BluetoothDeviceHandler()
{
    if(!sList.empty())
    {
        for(auto btDeviceList : sList)
        {
            delete btDeviceList;
        }
        sList.clear();
    }
}

bool BluetoothDeviceHandler::HandlerEvent(PdmNetlinkEvent* pNE){
    bool btDeviceProcessed = false;
    PDM_LOG_DEBUG("BluetoothDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
   if (pNE->getDevAttribute(ACTION) == "remove")
    {
      ProcessBluetoothDevice(pNE);
      return btDeviceProcessed;
    }
#ifdef WEBOS_SESSION
    if((pNE->getDevAttribute(ID_BLUETOOTH) == "1") || ((pNE->getDevAttribute(SUBSYSTEM) == "rfkill") && (pNE->getDevAttribute(RFKILL_NAME).find("hci") != std::string::npos)))
#else
    if(pNE->getDevAttribute(ID_BLUETOOTH) == "1")
#endif
    {
      ProcessBluetoothDevice(pNE);
      btDeviceProcessed = true;
    }
    return btDeviceProcessed;
}

void BluetoothDeviceHandler::removeDevice(BluetoothDevice* bluetoothDevice)
{
    sList.remove(bluetoothDevice);
    Notify(BLUETOOTH_DEVICE,REMOVE, bluetoothDevice);
    delete bluetoothDevice;
    bluetoothDevice = nullptr;
}

void BluetoothDeviceHandler::ProcessBluetoothDevice(PdmNetlinkEvent* pNE){
    BluetoothDevice *bluetoothDevice = nullptr;
    std::string deviceAction = pNE->getDevAttribute(ACTION);
    std::string deviceType = pNE->getDevAttribute(DEVTYPE);
    std::string devicePath = pNE->getDevAttribute(DEVPATH);
    PDM_LOG_INFO("BluetoothDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__,__LINE__,deviceType.c_str(),deviceAction.c_str());
 try {
        switch(sMapDeviceActions.at(pNE->getDevAttribute(ACTION)))
        {
            case DeviceActions::USB_DEV_ADD:
            bluetoothDevice = getDeviceWithPath< BluetoothDevice >(sList,devicePath);
            if(bluetoothDevice){
#ifdef WEBOS_SESSION
                PDM_LOG_DEBUG("BluetoothDeviceHandler:%s line: %d ACTION: DEVICE_ADD. Already present SUBSYSTEM: %s", __FUNCTION__, __LINE__, pNE->getDevAttribute(SUBSYSTEM).c_str());
                if (pNE->getDevAttribute(SUBSYSTEM) == "rfkill")
                    bluetoothDevice->updateDeviceInfo(pNE);
                Notify(BLUETOOTH_DEVICE,ADD, bluetoothDevice);
#else
                PDM_LOG_DEBUG("BluetoothDeviceHandler:%s line: %d ACTION: DEVICE_ADD. Already present", __FUNCTION__, __LINE__);
                bluetoothDevice->setDeviceInfo(pNE);
#endif
            } else {
                bluetoothDevice = new(std::nothrow) BluetoothDevice(m_pConfObj, m_pluginAdapter);
                if(bluetoothDevice){
                    bluetoothDevice->setDeviceInfo(pNE);
                    sList.push_back(bluetoothDevice);
#ifndef WEBOS_SESSION
                    Notify(BLUETOOTH_DEVICE,ADD);
#endif
            } else {
                PDM_LOG_ERROR("BluetoothDeviceHandler:%s line: %d Unable to instantiate the BluetoothDevice", __FUNCTION__, __LINE__);
            }
        }
      break;
        case DeviceActions::USB_DEV_REMOVE:
             bluetoothDevice = getDeviceWithPath< BluetoothDevice >(sList,devicePath);
          if(bluetoothDevice)
            removeDevice(bluetoothDevice);
             break;
        default:
            //Do nothing
            break;
        }
    }
    catch (const std::out_of_range& err) {
         PDM_LOG_INFO("BluetoothDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }

}

bool BluetoothDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse){

    PDM_LOG_DEBUG("BluetoothDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    return false;
}

bool BluetoothDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedDeviceStatus< BluetoothDevice >(sList, payload);
}

bool BluetoothDeviceHandler::GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedNonStorageDeviceList< BluetoothDevice >(sList, payload);
}
