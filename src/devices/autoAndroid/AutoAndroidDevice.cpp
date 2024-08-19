
// @@@LICENSE
//
// Copyright (c) 2020-2024 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@


#include "AutoAndroidDevice.h"
#include "PdmLogUtils.h"
#include "Common.h"

using namespace PdmDevAttributes;

void AutoAndroidDevice::setDeviceInfo(DeviceClass* devClass)
{
    PDM_LOG_DEBUG("AutoAndroidDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if(devClass->getAction()== DEVICE_ADD ) {
        PDM_LOG_DEBUG("AutoAndroidDevice:%s line: %d setDeviceInfo: DEVICE_ADD", __FUNCTION__, __LINE__);
        if(!devClass->getSpeed().empty()) {
            m_devSpeed = getDeviceSpeed(stoi(devClass->getSpeed(), nullptr));
        }
        Device::setDeviceInfo(devClass);
    }
}

void AutoAndroidDevice::registerCallback(handlerCb AutoAndroidDeviceHandlerCb) {
    mAutoAndroidDeviceHandlerCb = std::move(AutoAndroidDeviceHandlerCb);
}

