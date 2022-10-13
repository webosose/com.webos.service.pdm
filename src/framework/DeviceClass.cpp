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

#include "Common.h"
#include "DeviceClass.h"

DeviceClass::DeviceClass(std::unordered_map<std::string, std::string>& devPropMap)
{
	for(auto &prop : devPropMap)
		mDevPropMap[prop.first] = prop.second;
}

std::string DeviceClass::getDevType()
{
	return mDevPropMap[PdmDevAttributes::DEVTYPE];
}

std::string DeviceClass::getDevLinks()
{
	return mDevPropMap[PdmDevAttributes::DEVLINKS];
}

std::string DeviceClass::getSubsystemName()
{
	return mDevPropMap[PdmDevAttributes::SUBSYSTEM];
}
	
std::string DeviceClass::getAction()
{
	return mDevPropMap[PdmDevAttributes::ACTION];
}

std::string DeviceClass::getDevPath()
{
	return mDevPropMap[PdmDevAttributes::DEVPATH];
}

std::string DeviceClass::getInterfaceClass()
{
    std::string bInterfaceClass;
    if(!mDevPropMap[PdmDevAttributes::ID_USB_INTERFACES].empty())
        bInterfaceClass = mDevPropMap[PdmDevAttributes::ID_USB_INTERFACES];
    return bInterfaceClass;
}

std::string DeviceClass::getUsbDriver()
{
	return mDevPropMap[PdmDevAttributes::ID_USB_DRIVER];
}

std::string DeviceClass::getIdSerilShort()
{
	return mDevPropMap[PdmDevAttributes::ID_SERIAL_SHORT];
}

std::string DeviceClass::getIdModel()
{
	return mDevPropMap[PdmDevAttributes::ID_MODEL];
}

std::string DeviceClass::getIsPowerOnConnect()
{
	return mDevPropMap[PdmDevAttributes::IS_POWER_ON_CONNECT];
}

std::string DeviceClass::getIdVendorFromDataBase()
{
	return mDevPropMap[PdmDevAttributes::ID_VENDOR_FROM_DATABASE];
}

std::string DeviceClass::getIdVendor()
{
	return mDevPropMap[PdmDevAttributes::ID_VENDOR];
}

std::string DeviceClass::getIdProduct()
{
	return mDevPropMap[PdmDevAttributes::ID_PRODUCT_ID];
}

std::string DeviceClass::getDevNumber()
{
	return mDevPropMap[PdmDevAttributes::DEVNUM];
}

std::string DeviceClass::getDevName()
{
	return mDevPropMap[PdmDevAttributes::DEVNAME];
}

std::string DeviceClass::getHardDisk()
{
	return mDevPropMap[PdmDevAttributes::ID_ATA];
}

std::string DeviceClass::getMediaPlayerId()
{
	return mDevPropMap[PdmDevAttributes::ID_MEDIA_PLAYER];
}

std::string DeviceClass::getUsbInterfaceId()
{
	return mDevPropMap[PdmDevAttributes::ID_USB_INTERFACES];
}

std::string DeviceClass::getGamepadId()
{
	return mDevPropMap[PdmDevAttributes::ID_GAMEPAD];
}

std::string DeviceClass::getUsbModemId()
{
	return mDevPropMap[PdmDevAttributes::ID_USB_MODEM_DONGLE];
}

std::string DeviceClass::getUsbSerialId()
{
	return mDevPropMap[PdmDevAttributes::ID_USB_SERIAL];
}

std::string DeviceClass::getUsbSerialSubType()
{
	return mDevPropMap[PdmDevAttributes::USB_SERIAL_SUB_TYPE];
}

std::string DeviceClass::getUsbInterfaces()
{
	return mDevPropMap[PdmDevAttributes::ID_USB_INTERFACES];
}

std::string DeviceClass::getBluetoothId()
{
	return mDevPropMap[PdmDevAttributes::ID_BLUETOOTH];
}

#ifdef WEBOS_SESSION
std::string DeviceClass::getUsbPort()
{
	return mDevPropMap[PdmDevAttributes::USB_PORT];
}

std::string DeviceClass::getRfKillName()
{
	return mDevPropMap[PdmDevAttributes::RFKILL_NAME];
}
#endif

std::string DeviceClass::getBusType()
{
	//ToDo
	return "";
}

std::string DeviceClass::getBusNum()
{
	return mDevPropMap[PdmDevAttributes::BUSNUM];
}


std::string DeviceClass::getModelId()
{
	return mDevPropMap[PdmDevAttributes::ID_MODEL_ID];
}

std::string DeviceClass::getSpeed()
{
	return mDevPropMap[PdmDevAttributes::SPEED];
}

std::string DeviceClass::getNetIfIndex()
{
	return mDevPropMap[PdmDevAttributes::NET_IFIINDEX];
}

std::string DeviceClass::getNetLinkMode()
{
	return mDevPropMap[PdmDevAttributes::NET_LINK_MODE];
}

std::string DeviceClass::getNetDuplex()
{
	return mDevPropMap[PdmDevAttributes::NET_DUPLEX];
}

std::string DeviceClass::getNetAddress()
{
	return mDevPropMap[PdmDevAttributes::NET_ADDRESS];
}

std::string DeviceClass::getNetOperState()
{
	return mDevPropMap[PdmDevAttributes::NET_OPERSTATE];
}

std::string DeviceClass::getProcessed()
{
	return mDevPropMap[PdmDevAttributes::PROCESSED];
}

std::string DeviceClass::isCardReader()
{
	return mDevPropMap[PdmDevAttributes::CARD_READER];
}

std::string DeviceClass::isHardDisk()
{
	return mDevPropMap[PdmDevAttributes::HARD_DISK];
}

std::string DeviceClass::getIdInstance()
{
	return mDevPropMap[PdmDevAttributes::ID_INSTANCE];
}

std::string DeviceClass::getIdBlackListedSuperSpeedDev()
{
	return mDevPropMap[PdmDevAttributes::ID_BLACK_LISTED_SUPER_SPEED_DEVICE];
}

std::string DeviceClass::isReadOnly()
{
	return mDevPropMap[PdmDevAttributes::READ_ONLY];
}

std::string DeviceClass::isDiskMediaChange()
{
	return mDevPropMap[PdmDevAttributes::DISK_MEDIA_CHANGE];
}

std::string DeviceClass::getFsType()
{
	return mDevPropMap[PdmDevAttributes::ID_FS_TYPE];
}

std::string DeviceClass::getFsUuid()
{
	return mDevPropMap[PdmDevAttributes::ID_FS_UUID];
}

std::string DeviceClass::getFsLabelEnc()
{
	return mDevPropMap[PdmDevAttributes::ID_FS_LABEL_ENC];
}

