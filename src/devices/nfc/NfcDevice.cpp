
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


#include "NfcDevice.h"
#include "PdmLogUtils.h"
#include "Common.h"

using namespace PdmDevAttributes;

void NfcDevice::setDeviceInfo(DeviceClass* devClass)
{
    if(devClass->getAction() == DEVICE_ADD ) {
        if (!devClass->getSpeed().empty()) {
            m_devSpeed = getDeviceSpeed(stoi(devClass->getSpeed(), nullptr));
        }
        Device::setDeviceInfo(devClass);
    }
}

void NfcDevice::registerCallback(handlerCb nfcDeviceHandlerCb) {
    mnfcDeviceHandlerCb = std::move(nfcDeviceHandlerCb);
}

