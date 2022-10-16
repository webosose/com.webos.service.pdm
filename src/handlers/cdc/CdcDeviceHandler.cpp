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

#include "Common.h"
#include "CdcDeviceHandler.h"
#include "PdmJson.h"
#include "CdcSubSystem.h"

using namespace PdmDevAttributes;

bool CdcDeviceHandler::mIsObjRegistered = CdcDeviceHandler::RegisterObject();

CdcDeviceHandler::CdcDeviceHandler(PdmConfig *const pConfObj, PluginAdapter *const pluginAdapter) : DeviceHandler(pConfObj, pluginAdapter), m_is3g4gDongleSupported(true), m_deviceRemoved(false)
{
    lunaHandler->registerLunaCallback(std::bind(&CdcDeviceHandler::GetAttachedDeviceStatus, this, _1, _2),
                                      GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&CdcDeviceHandler::GetAttachedNonStorageDeviceList, this, _1, _2),
                                      GET_NONSTORAGEDEVICELIST);
    lunaHandler->registerLunaCallback(std::bind(&CdcDeviceHandler::GetAttachedNetDeviceList, this, _1, _2),
                                      GET_NETDEVICELIST);
}

CdcDeviceHandler::~CdcDeviceHandler()
{
    if (!sList.empty())
    {
        for (auto cdcDevice : sList)
        {
            delete cdcDevice;
        }
        sList.clear();
    }
}

bool CdcDeviceHandler::HandlerEvent(DeviceClass *devClass)
{
    PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d ", __FUNCTION__, __LINE__);

    if (devClass->getAction() == "remove")
    {
        m_deviceRemoved = false;
        ProcessCdcDevice(devClass);
        if (m_deviceRemoved)
        {
            PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d  DEVTYPE=usb_device removed", __FUNCTION__, __LINE__);
            return true;
        }
    }
    if (!identifyCdcDevice(devClass))
        return false;
    if (devClass->getDevType() == USB_DEVICE)
    {
        ProcessCdcDevice(devClass);
        return false;
    }
    else if ((devClass->getSubsystemName() == "net") || (devClass->getSubsystemName() == "tty"))
    {
        ProcessCdcDevice(devClass);
        return true;
    }
    return false;
}

void CdcDeviceHandler::removeDevice(CdcDevice *cdcDevice)
{
    if (!cdcDevice)
        return;
    sList.remove(cdcDevice);
    Notify(CDC_DEVICE, REMOVE, cdcDevice);
    delete cdcDevice;
    cdcDevice = nullptr;
}

void CdcDeviceHandler::ProcessCdcDevice(DeviceClass *devClass)
{
    CdcSubSystem *cdcSubsystem = (CdcSubSystem *)devClass;

    CdcDevice *cdcDevice;
    PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d CdcDeviceHandler: DEVTYPE: %s ACTION: %s", __FUNCTION__, __LINE__, devClass->getDevType().c_str(), devClass->getAction().c_str());
    try
    {
        switch (sMapDeviceActions.at(devClass->getAction()))
        {
        case DeviceActions::USB_DEV_ADD:
            PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d  Add CDC device", __FUNCTION__, __LINE__);
            cdcDevice = getDeviceWithPath<CdcDevice>(sList, devClass->getDevPath());
            if (!cdcDevice)
            {
                cdcDevice = new (std::nothrow) CdcDevice(m_pConfObj, m_pluginAdapter);
                if (cdcDevice)
                {
                    cdcDevice->setDeviceInfo(devClass);
                    sList.push_back(cdcDevice);
                    if (cdcSubsystem->getUsbModemId() == YES)
                    { // In case of modem dongle there is only a single event and no update happens later.
                        sList.push_back(cdcDevice);
                        Notify(CDC_DEVICE, ADD); // So notify now itself
                    }
                }
                else
                {
                    PDM_LOG_CRITICAL("CdcDeviceHandler:%s line: %d Unable to create new CDC device", __FUNCTION__, __LINE__);
                }
            }
            else
            {
                sList.push_back(cdcDevice);
                cdcDevice->updateDeviceInfo(devClass);
                Notify(CDC_DEVICE, ADD, cdcDevice);
            }
            break;
        case DeviceActions::USB_DEV_REMOVE:
            cdcDevice = getDeviceWithPath<CdcDevice>(sList, devClass->getDevPath());
            if (cdcDevice)
            {
                removeDevice(cdcDevice);
                m_deviceRemoved = true;
            }
            break;
        default:
            // Do nothing
            break;
        }
    }
    catch (const std::out_of_range &err)
    {
        PDM_LOG_INFO("StorageDeviceHandler:", 0, "%s line: %d out of range : %s", __FUNCTION__, __LINE__, err.what());
    }
}

bool CdcDeviceHandler::identifyCdcDevice(DeviceClass *devClass)
{
    CdcSubSystem *cdcSubsystem = (CdcSubSystem *)devClass;

    std::string interfaceClass = devClass->getInterfaceClass();

    if ((interfaceClass.find(iClass) != std::string::npos) ||
        (cdcSubsystem->getUsbSerialId() == YES) ||
        ((devClass->getInterfaceClass().find(ethernetInterfaceClass)) != std::string::npos) ||
        ((cdcSubsystem->getUsbModemId() == YES) && m_is3g4gDongleSupported))
        return true;

    return false;
}

bool CdcDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse)
{
    PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    return false;
}

bool CdcDeviceHandler::HandlePluginEvent(int eventType)
{
    switch (eventType)
    {
    case MODEM_DONGLE_SUPPORTED:
        m_is3g4gDongleSupported = true;
        break;
    case MODEM_DONGLE_NOT_SUPPORTED:
        m_is3g4gDongleSupported = false;
        break;
    default:
        break;
    }
    return false;
}

bool CdcDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedDeviceStatus<CdcDevice>(sList, payload);
}

bool CdcDeviceHandler::GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedNonStorageDeviceList<CdcDevice>(sList, payload);
}

bool CdcDeviceHandler::GetAttachedNetDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedNetDeviceList<CdcDevice>(sList, payload);
}