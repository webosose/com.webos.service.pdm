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

#ifndef _DEVICE_H
#define _DEVICE_H


#include <string>
#include "Common.h"
#include "IDevice.h"
#include "PdmConfig.h"
#include "PdmNetlinkEvent.h"
#include "PluginAdapter.h"

enum DeviceSpeed { FULL = 12, HIGH = 480, SUPER = 5000};

enum DeviceActions {USB_DEV_ADD =0, USB_DEV_REMOVE, USB_DEV_CHANGE};
enum UsbDeviceTypes {TYPE_DEV_USB =0, TYPE_DEV_DISK, TYPE_DEV_PARTITION};

// Map to associate storage device type with the UsbDeviceTypes enum values
static std::map<std::string, UsbDeviceTypes> sMapUsbDeviceType= {
    {"usb_device", TYPE_DEV_USB},
    {"disk",       TYPE_DEV_DISK},
    {"partition",  TYPE_DEV_PARTITION}
};

// Map to associate device actions with the DeviceActions enum values
static std::map<std::string, DeviceActions> sMapDeviceActions= {
    {"add",     USB_DEV_ADD},
    {"remove",  USB_DEV_REMOVE},
    {"change",  USB_DEV_CHANGE}
};

class Device : public IDevice {

protected:

    bool m_isPowerOnConnect;
    bool m_isToastRequired;
    int m_deviceNum;
    int m_usbPortNum;
    int m_busNum;
    int m_usbPortSpeed;

    PdmConfig* const m_pConfObj;
    PluginAdapter* const m_pluginAdapter;
    std::string m_deviceStatus;
    std::string m_deviceType;
    std::string m_errorReason;
    std::string m_deviceName;
    std::string m_devicePath;
    std::string m_serialNumber;
    std::string m_vendorName;
    std::string m_productName;
    std::string m_devSpeed;
    std::string m_deviceSubType;
#ifdef WEBOS_SESSION
    std::string m_devPath;
    std::string m_vendorID;
    std::string m_productID;
    std::string m_hubPortNumber;
    std::string m_deviceSetId;
#endif

public:

    Device(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter, std::string deviceType, std::string errorReason);
    virtual ~Device(){}
    virtual int getUsbPortNumber(){return m_usbPortNum;}
    virtual int getDeviceNum() { return m_deviceNum; }
    virtual std::string getDeviceStatus() { return m_deviceStatus; }
    virtual std::string getDeviceType(){return m_deviceType;}
    virtual std::string getErrorReason(){return m_errorReason;}
    virtual std::string getVendorName(){return m_vendorName;}
    virtual std::string getProductName(){return m_productName;}
    virtual std::string getSerialNumber(){return m_serialNumber;}
    virtual std::string getDevSpeed(){return m_devSpeed;}
    virtual std::string getDevicePath(){return m_devicePath;}
    virtual std::string getDeviceSubType(){return m_deviceSubType;}
    virtual std::string getDeviceName(){ return m_deviceName;}
    virtual bool isConnectedToPower(){return m_isPowerOnConnect;}
    virtual bool canDisplayToast(){return m_isToastRequired;}
    virtual std::string getDeviceSpeed(int speed)const;
    void setDeviceInfo(PdmNetlinkEvent* pNE);
    void setProductName(const std::string& productName){m_productName = productName;}
    void onDeviceRemove(){}
    virtual int getPortSpeed(){return m_usbPortSpeed;}
    virtual void setDeviceType(std::string devType){m_deviceType = devType;}
    virtual int getBusNumber(){return m_busNum;}
#ifdef WEBOS_SESSION
    virtual std::string getDevPath(){ return m_devPath;}
    virtual std::string getVendorID() {return m_vendorID;}
    virtual std::string getProductID() {return m_productID;}
    virtual std::string getHubPortNumber() {return m_hubPortNumber;}
    virtual std::string getDeviceSetId() {return m_deviceSetId;}
    void setDeviceSetId(std::string hubPortPath);
    bool readFromFile(std::string fileToRead, std::string &usbData);
    void getBasicUsbInfo(std::string devPath);
#endif
};
#endif //_DEVICE_H
