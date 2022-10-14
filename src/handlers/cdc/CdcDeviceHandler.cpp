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
    CdcSubSystem *cdcSubsystem = (CdcSubSystem *)devClass;
    if (cdcSubsystem == nullptr) return false;

    PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d ", __FUNCTION__, __LINE__);

    if (cdcSubsystem->getAction() == "remove")
    {
        m_deviceRemoved = false;
        ProcessCdcDevice(cdcSubsystem);
        if (m_deviceRemoved)
        {
            PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d  DEVTYPE=usb_device removed", __FUNCTION__, __LINE__);
            return true;
        }
    }
    if (!identifyCdcDevice(cdcSubsystem))
        return false;
    if (cdcSubsystem->getDevType() == USB_DEVICE)
    {
        ProcessCdcDevice(cdcSubsystem);
        return false;
    }
    else if ((cdcSubsystem->getSubsystemName() == "net") || (cdcSubsystem->getSubsystemName() == "tty"))
    {
        ProcessCdcDevice(cdcSubsystem);
        return true;
    }
    return false;
}

#if 0
bool CdcDeviceHandler::HandlerEvent(PdmNetlinkEvent* pNE)
{
    PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d ", __FUNCTION__, __LINE__);

    if (pNE->getDevAttribute(ACTION) == "remove")
    {
        m_deviceRemoved = false;
        ProcessCdcDevice(pNE);
        if(m_deviceRemoved) {
            PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d  DEVTYPE=usb_device removed", __FUNCTION__, __LINE__);
            return true;
         }
    }
    if(!identifyCdcDevice(pNE))
        return false;
    if(pNE->getDevAttribute(DEVTYPE) ==  USB_DEVICE) {
        ProcessCdcDevice(pNE);
        return false;
    }
    else if((pNE->getDevAttribute(SUBSYSTEM) ==  "net") || (pNE->getDevAttribute(SUBSYSTEM) ==  "tty")) {
        ProcessCdcDevice(pNE);
        return true;
    }
    return false;
}
#endif

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

#if 0
void CdcDeviceHandler::ProcessCdcDevice(PdmNetlinkEvent* pNE)
{
    CdcDevice *cdcDevice;
    PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d CdcDeviceHandler: DEVTYPE: %s ACTION: %s", __FUNCTION__, __LINE__,pNE->getDevAttribute(DEVTYPE).c_str(),pNE->getDevAttribute(ACTION).c_str());
     try {
            switch(sMapDeviceActions.at(pNE->getDevAttribute(ACTION)))
            {
                case DeviceActions::USB_DEV_ADD:
                PDM_LOG_DEBUG("CdcDeviceHandler:%s line: %d  Add CDC device",__FUNCTION__, __LINE__);
                cdcDevice = getDeviceWithPath< CdcDevice >(sList,pNE->getDevAttribute(DEVPATH));
                if(!cdcDevice ) {
                   cdcDevice = new (std::nothrow) CdcDevice(m_pConfObj, m_pluginAdapter);
                   if(cdcDevice) {
                      cdcDevice->setDeviceInfo(pNE);
                      sList.push_back(cdcDevice);
                      if(pNE->getDevAttribute(ID_USB_MODEM_DONGLE) == YES) { // In case of modem dongle there is only a single event and no update happens later.
                          sList.push_back(cdcDevice);
                          Notify(CDC_DEVICE,ADD); // So notify now itself
                      }
                   } else {
                      PDM_LOG_CRITICAL("CdcDeviceHandler:%s line: %d Unable to create new CDC device", __FUNCTION__, __LINE__);
                   }
                } else{
                   sList.push_back(cdcDevice);
                   cdcDevice->updateDeviceInfo(pNE);
                   Notify(CDC_DEVICE,ADD,cdcDevice);
                }
                break;
                case DeviceActions::USB_DEV_REMOVE:
                    cdcDevice = getDeviceWithPath< CdcDevice >(sList,pNE->getDevAttribute(DEVPATH));
                if(cdcDevice) {
                   removeDevice(cdcDevice);
                   m_deviceRemoved = true;
                }
                    break;
                default:
                 //Do nothing
                    break;
            }
        }
        catch (const std::out_of_range& err) {
         PDM_LOG_INFO("StorageDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }
}
#endif

bool CdcDeviceHandler::identifyCdcDevice(DeviceClass *devClass)
{
    CdcSubSystem *cdcSubsystem = (CdcSubSystem *)devClass;
    if (cdcSubsystem == nullptr) return false;

    std::string interfaceClass = devClass->getInterfaceClass();

    if ((interfaceClass.find(iClass) != std::string::npos) ||
        (cdcSubsystem->getUsbSerialId() == YES) ||
        ((devClass->getInterfaceClass().find(ethernetInterfaceClass)) != std::string::npos) ||
        ((cdcSubsystem->getUsbModemId() == YES) && m_is3g4gDongleSupported))
        return true;

    return false;
}

#if 0
bool CdcDeviceHandler::identifyCdcDevice(PdmNetlinkEvent* pNE)
{
    std::string interfaceClass = pNE->getInterfaceClass();

    if((interfaceClass.find(iClass) != std::string::npos) ||
    (pNE->getDevAttribute(ID_USB_SERIAL) == YES) ||
    ((pNE->getDevAttribute(ID_USB_INTERFACES).find(ethernetInterfaceClass)) != std::string::npos) ||
    ((pNE->getDevAttribute(ID_USB_MODEM_DONGLE) == YES) && m_is3g4gDongleSupported))
        return true;

    return false;
}
#endif

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
