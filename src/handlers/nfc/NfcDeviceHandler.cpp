// @@@LICENSE
//
// Copyright (c) 2020-2022 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@

#include "NfcDeviceHandler.h"
#include "PdmJson.h"
#include "NfcSubsystem.h"

using namespace PdmDevAttributes;

bool NfcDeviceHandler::mIsObjRegistered = NfcDeviceHandler::RegisterObject();

NfcDeviceHandler::NfcDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
                        : DeviceHandler(pConfObj, pluginAdapter)
                        , m_deviceRemoved(false)

{
    lunaHandler->registerLunaCallback(std::bind(&NfcDeviceHandler::GetAttachedDeviceStatus, this, _1, _2),
                                                                          GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&NfcDeviceHandler::GetAttachedNonStorageDeviceList, this, _1, _2),
                                                                                    GET_NONSTORAGEDEVICELIST);
}

NfcDeviceHandler::~NfcDeviceHandler() {
}

bool NfcDeviceHandler::HandlerEvent(DeviceClass* devClass)
{
    NfcSubsystem *nfcSubsystem = (NfcSubsystem *)devClass;
    if (nfcSubsystem == nullptr) return false;

    if (nfcSubsystem->getAction()== "remove")
    {
      ProcessNfcDevice(nfcSubsystem);
      if(m_deviceRemoved)
          return true;
    }

    if (nfcSubsystem->getInterfaceClass().find(iClass) != std::string::npos) {
        ProcessNfcDevice(nfcSubsystem);
        return true;
    }
    return false;
}

#if 0
bool NfcDeviceHandler::HandlerEvent(PdmNetlinkEvent* pNE){

    if (pNE->getDevAttribute(ACTION) == "remove")
    {
      ProcessNfcDevice(pNE);
      if(m_deviceRemoved)
          return true;
    }

    if (pNE->getDevAttribute(ID_USB_INTERFACES).find(iClass) != std::string::npos) {
        ProcessNfcDevice(pNE);
        return true;
    }
    return false;
}
#endif

void NfcDeviceHandler::removeDevice(NfcDevice* device)
{
    if(!device)
        return;
    sList.remove(device);
    Notify(NFC_DEVICE,REMOVE,device);
    delete device;
    device = nullptr;

}

void NfcDeviceHandler::ProcessNfcDevice(DeviceClass* devClass){

    PDM_LOG_INFO("NfcDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__, __LINE__, devClass->getDevType().c_str(), devClass->getAction().c_str());
    NfcDevice* nfcDevice = nullptr;
    try {
        switch(sMapDeviceActions.at(devClass->getAction()))
        {
            case DeviceActions::USB_DEV_ADD:
                nfcDevice = getDeviceWithPath< NfcDevice >(sList, devClass->getDevPath());
                if(!nfcDevice)
                {
                    nfcDevice = new (std::nothrow) NfcDevice(m_pConfObj, m_pluginAdapter);
                    if(!nfcDevice)
                        break;
                    nfcDevice->setDeviceInfo(devClass);
                    nfcDevice->registerCallback(std::bind(&NfcDeviceHandler::commandNotification, this, _1, _2));
                    sList.push_back(nfcDevice);
                    Notify(NFC_DEVICE,ADD, nfcDevice);
                }else
                    nfcDevice->setDeviceInfo(devClass);
                break;
            case DeviceActions::USB_DEV_REMOVE:
                nfcDevice = getDeviceWithPath< NfcDevice >(sList, devClass->getDevPath());
                if(nfcDevice) {
                    removeDevice(nfcDevice);
                    m_deviceRemoved = true;
                }
                break;
            default:
                //Do nothing
                break;
        }
    }
    catch (const std::out_of_range& err) {
         PDM_LOG_INFO("NfcDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }
}

#if 0
void NfcDeviceHandler::ProcessNfcDevice(PdmNetlinkEvent* pNE){

    PDM_LOG_INFO("NfcDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__,__LINE__,pNE->getDevAttribute(DEVTYPE).c_str(),pNE->getDevAttribute(ACTION).c_str());
    NfcDevice* nfcDevice = nullptr;
    try {
        switch(sMapDeviceActions.at(pNE->getDevAttribute(ACTION)))
        {
            case DeviceActions::USB_DEV_ADD:
                nfcDevice = getDeviceWithPath< NfcDevice >(sList,pNE->getDevAttribute(DEVPATH));
                if(!nfcDevice)
                {
                    nfcDevice = new (std::nothrow) NfcDevice(m_pConfObj, m_pluginAdapter);
                    if(!nfcDevice)
                        break;
                    nfcDevice->setDeviceInfo(pNE);
                    nfcDevice->registerCallback(std::bind(&NfcDeviceHandler::commandNotification, this, _1, _2));
                    sList.push_back(nfcDevice);
                    Notify(NFC_DEVICE,ADD, nfcDevice);
                }else
                    nfcDevice->setDeviceInfo(pNE);
                break;
            case DeviceActions::USB_DEV_REMOVE:
                nfcDevice = getDeviceWithPath< NfcDevice >(sList,pNE->getDevAttribute(DEVPATH));
                if(nfcDevice) {
                    removeDevice(nfcDevice);
                    m_deviceRemoved = true;
                }
                break;
            default:
                //Do nothing
                break;
        }
    }
    catch (const std::out_of_range& err) {
         PDM_LOG_INFO("NfcDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }
}
#endif

bool NfcDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) {

    PDM_LOG_DEBUG("NfcDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    return false;
}

bool NfcDeviceHandler::HandlePluginEvent(int eventType)
{
    return true;
}

bool NfcDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedDeviceStatus< NfcDevice >(sList, payload );
}

bool NfcDeviceHandler::GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
   return getAttachedNonStorageDeviceList< NfcDevice >( sList, payload );
}

void NfcDeviceHandler::commandNotification(EventType event, NfcDevice* device)
{

    Notify(NFC_DEVICE,event,device);

}
