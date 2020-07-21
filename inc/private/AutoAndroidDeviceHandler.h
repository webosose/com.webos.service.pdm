// @@@LICENSE
//
// Copyright (c) 2020 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@

#ifndef _AUTOANDROIDDEVICEHANDLER_H_
#define _AUTOANDROIDDEVICEHANDLER_H_

#include "CommandManager.h"
#include "DeviceHandler.h"
#include "AutoAndroidDevice.h"
#include "PdmDeviceFactory.h"
#include "PdmNetlinkEvent.h"
#include "PdmLogUtils.h"
#include <libusb.h>

#define ACCESSORY_STRING_MANUFACTURER 0
#define ACCESSORY_STRING_MODEL 1
#define ACCESSORY_STRING_DESCRIPTION 2
#define ACCESSORY_STRING_VERSION 3
#define ACCESSORY_STRING_URI 4
#define ACCESSORY_STRING_SERIAL 5
#define ACCESSORY_GET_PROTOCOL 51U
#define ACCESSORY_SEND_STRING 52U
#define ACCESSORY_START 53U

#define ACCESSORY_MANUFACTURER_NAME "Android"
#define ACCESSORY_MODEL_NAME "Android Open Automotive Protocol"
#define ACCESSORY_DESCRIPTION "Android Open Automotive Protocol"
#define ACCESSORY_VERSION "1.0"
#define ACCESSORY_URI "http://www.android.com/"
#define ACCESSORY_SERIAL_NUMBER "0000000012345678"

#define HOST_TO_DEVICE_TYPE 0xc0U
#define DEVICE_TO_HOST_TYPE 0x40U
#define USB_OPEN_RETRY_COUNT 10
#define USB_OPEN_RETRY_USECOND 1000U * 100U

class AutoAndroidDeviceHandler : public DeviceHandler
{
private:
    libusb_device_handle* mHandle;
    std::list<AutoAndroidDevice*> sList;
    bool m_deviceRemoved;
    libusb_context* m_context;

    AutoAndroidDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    static bool mIsObjRegistered;

    //Register Object to object factory. This is called automatically
    static bool RegisterObject() {
        return (PdmDeviceFactory::getInstance()->Register("AUTOANDROID",
                                                          &AutoAndroidDeviceHandler::CreateObject));
    }
    bool isAOAInterface(PdmNetlinkEvent* pNE);
    bool isAOAProductId(PdmNetlinkEvent* pNE);
    int startAccessoryMode();
    int getAOAPProtocol();
    void removeDevice(AutoAndroidDevice* Device);
    bool openDevice(PdmNetlinkEvent* pNE);

public:
    ~AutoAndroidDeviceHandler();

    static DeviceHandler* CreateObject(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter) {
        if (mIsObjRegistered) {
            PDM_LOG_DEBUG("CreateObject - Creating the HID Device");
            return new AutoAndroidDeviceHandler(pConfObj, pluginAdapter);
        } else {
            return nullptr;
        }
    }

    bool HandlerEvent(PdmNetlinkEvent* pNE) override;
    bool HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) override;
    bool HandlePluginEvent(int eventType) override;
    bool GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message) override;
    bool GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message);
    bool GetExampleAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message);
    void ProcessAutoAndroidDevice(PdmNetlinkEvent* pNE);
    void commandNotification(EventType event, AutoAndroidDevice* device);
};

#endif // AUTOANDROIDDEVICEHANDLER_H_
