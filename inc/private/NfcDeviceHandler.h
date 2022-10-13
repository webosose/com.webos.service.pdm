// @@@LICENSE
//
// Copyright (c) 2020-2021 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@

#ifndef _NFCDEVICEHANDLER_H_
#define _NFCDEVICEHANDLER_H_

#include "CommandManager.h"
#include "DeviceHandler.h"
#include "NfcDevice.h"
#include "PdmDeviceFactory.h"
#include "PdmLogUtils.h"
#include "DeviceClass.h"

class NfcDeviceHandler : public DeviceHandler
{
private:
    const std::string iClass = ":0b";
    std::list<NfcDevice*> sList;
    bool m_deviceRemoved;

    NfcDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    static bool mIsObjRegistered;

    //Register Object to object factory. This is called automatically
    static bool RegisterObject() {
        return (PdmDeviceFactory::getInstance()->Register("NFC",
                                                          &NfcDeviceHandler::CreateObject));
    }
    void removeDevice(NfcDevice* Device);

public:
    ~NfcDeviceHandler();

    static DeviceHandler* CreateObject(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) {
        if (mIsObjRegistered) {
            PDM_LOG_DEBUG("CreateObject - Creating the NFC Device");
            return new NfcDeviceHandler(pConfObj, pluginAdapter);
        } else {
            return nullptr;
        }
    }

    bool HandlerEvent(DeviceClass* deviceClass) override;
    //bool HandlerEvent(PdmNetlinkEvent* pNE) override;
    bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) override;
    bool HandlePluginEvent(int eventType) override;
    bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) override;
    bool GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message);
    void ProcessNfcDevice(DeviceClass*);
	//void ProcessNfcDevice(PdmNetlinkEvent* pNE);
    void commandNotification(EventType event, NfcDevice* device);
};

#endif // AUTOANDROIDDEVICEHANDLER_H_
