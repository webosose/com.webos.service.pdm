// Copyright (c) 2022 LG Electronics, Inc.
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

#ifndef DEVICECLASS_H_
#define DEVICECLASS_H_

#include <libudev.h>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

#include "Common.h"

class BusType{
};

class MipiCsi : public BusType
{
};

class Usb : public BusType
{
};

const std::string ID_V4L_CAPABILITIES  =  "ID_V4L_CAPABILITIES";
const std::string ID_V4L_PRODUCT  =  "ID_V4L_PRODUCT";
const std::string ID_V4L_VERSION  =  "ID_V4L_VERSION";
const std::string CARD_NAME  =  "CARD_NAME";
// const std::string ID_USB_INTERFACES  =  "ID_USB_INTERFACES";

class DeviceClass
{
	//BusType mBusType;
	std::unordered_map<std::string, std::string> mDevPropMap;
public:
	DeviceClass() = default;
	DeviceClass(std::unordered_map<std::string, std::string>&);
	virtual std::string getDevType();
	virtual std::string getDevLinks();
	virtual std::string getSubsystemName();
	virtual std::string getAction();
	virtual std::string getDevPath();
	virtual std::string getBusType();
	virtual std::string getBusNum();
	virtual std::string getInterfaceClass();
	virtual std::string getUsbDriver();
	virtual std::string getIdSerilShort();
	virtual std::string getIdModel();
	virtual std::string getIdProduct();
	virtual std::string getIsPowerOnConnect();
	virtual std::string getIdVendorFromDataBase();
	virtual std::string getIdVendor();
	virtual std::string getDevNumber();
	virtual std::string getDevName();
	virtual std::string getHardDisk();
	virtual std::string getMediaPlayerId();
	virtual std::string getUsbInterfaceId();
	virtual std::string getGamepadId();
	virtual std::string getUsbModemId();
	virtual std::string getUsbSerialId();
	virtual std::string getUsbSerialSubType();
	virtual std::string getUsbInterfaces();
	virtual std::string getBluetoothId();
	virtual std::string getModelId();
	virtual std::string getSpeed();
	virtual std::string getNetIfIndex();
	virtual std::string getNetLinkMode();
	virtual std::string getNetDuplex();
	virtual std::string getNetAddress();
	virtual std::string getNetOperState();
	virtual std::string getProcessed();
	virtual std::string isCardReader();
	virtual std::string isHardDisk();
	virtual std::string getIdInstance();
	virtual std::string getIdBlackListedSuperSpeedDev();
	virtual std::string isReadOnly();
	virtual std::string isDiskMediaChange();
	virtual std::string getFsType();
	virtual std::string getFsUuid();
	virtual std::string getFsLabelEnc();
	
#ifdef WEBOS_SESSION
	virtual std::string getUsbPort();
	virtual std::string getRfKillName();
#endif
};

#endif /* DEVICECLASS_H_ */
