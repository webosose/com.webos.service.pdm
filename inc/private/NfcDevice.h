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


#ifndef _nfcDevice_H_
#define _nfcDevice_H_

#include "Device.h"
#include "DeviceClass.h"
#include <functional>

class nfcDeviceHandler;

class NfcDevice : public Device
{
private:
    using handlerCb = std::function<void(EventType,NfcDevice*)>;
    handlerCb mnfcDeviceHandlerCb;
    bool m_isDevAddNotified;
	std::string m_deviceClass;
public:
    NfcDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
              : Device(pConfObj, pluginAdapter, "NFC",PdmDevAttributes::PDM_ERR_NOTHING), m_isDevAddNotified(false), m_deviceClass(""){}
    ~NfcDevice() = default;
    void setDeviceInfo(DeviceClass*);
	//void setDeviceInfo(PdmNetlinkEvent* pNE);
    void registerCallback(handlerCb nfcDeviceHandlerCb);
	std::string getDeviceClass(){return m_deviceClass;}
};

#endif // nfcDevice_H
