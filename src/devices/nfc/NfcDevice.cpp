
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


#include "NfcDevice.h"
#include "PdmLogUtils.h"
#include "Common.h"

using namespace PdmDevAttributes;

void NfcDevice::setDeviceInfo(DeviceClass* devClass)
{
    PDM_LOG_DEBUG("NfcDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if(devClass->getAction() == DEVICE_ADD ) {
        PDM_LOG_DEBUG("NfcDevice:%s line: %d setDeviceInfo: DEVICE_ADD", __FUNCTION__, __LINE__);
        if (!devClass->getSpeed().empty()) {
            m_devSpeed = getDeviceSpeed(stoi(devClass->getSpeed(), nullptr));
        }
        Device::setDeviceInfo(devClass);
    }
}

#if 0
void NfcDevice::setDeviceInfo(PdmNetlinkEvent* pNE)
{
    PDM_LOG_DEBUG("NfcDevice:%s line: %d setDeviceInfo", __FUNCTION__, __LINE__);
    if(pNE->getDevAttribute(ACTION) == DEVICE_ADD ) {
        PDM_LOG_DEBUG("NfcDevice:%s line: %d setDeviceInfo: DEVICE_ADD", __FUNCTION__, __LINE__);
        if (!pNE->getDevAttribute(SPEED).empty()) {
            m_devSpeed = getDeviceSpeed(stoi(pNE->getDevAttribute(SPEED),nullptr));
        }
        Device::setDeviceInfo(pNE);
    }
}
#endif

void NfcDevice::registerCallback(handlerCb nfcDeviceHandlerCb) {
    mnfcDeviceHandlerCb = nfcDeviceHandlerCb;
}

