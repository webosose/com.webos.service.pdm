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


#ifndef _AUTOANDROIDDEVICE_H_
#define _AUTOANDROIDDEVICE_H_

#include "Device.h"
#include <functional>

class AutoAndroidDeviceHandler;

class AutoAndroidDevice : public Device
{
private:
    using handlerCb = std::function<void(EventType,AutoAndroidDevice*)>;
    handlerCb mAutoAndroidDeviceHandlerCb;
    bool m_isDevAddNotified;
public:
    AutoAndroidDevice(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
              : Device(pConfObj, pluginAdapter, "AUTOANDROID",PdmDevAttributes::PDM_ERR_NOTHING), m_isDevAddNotified(false){}
    ~AutoAndroidDevice() = default;
    void setDeviceInfo(PdmNetlinkEvent* pNE);
    void registerCallback(handlerCb AutoAndroidDeviceHandlerCb);
};

#endif // AutoAndroidDevice_H
