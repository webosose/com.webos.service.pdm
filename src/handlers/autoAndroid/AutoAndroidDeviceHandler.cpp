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

#include "AutoAndroidDeviceHandler.h"
#include "PdmJson.h"
#include <string.h>

static int sendString(libusb_device_handle* handle, const char* str, int index) {
  return libusb_control_transfer(handle, DEVICE_TO_HOST_TYPE, ACCESSORY_SEND_STRING, 0U, static_cast<uint16_t>(index),
                                 (uint8_t*)str, static_cast<uint16_t>(strlen(str)) + 1U, 3000U);
}

using namespace PdmDevAttributes;

bool AutoAndroidDeviceHandler::mIsObjRegistered = AutoAndroidDeviceHandler::RegisterObject();

AutoAndroidDeviceHandler::AutoAndroidDeviceHandler(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter)
                        : DeviceHandler(pConfObj, pluginAdapter)
                        , m_deviceRemoved(false)
	                , m_context(nullptr)

{
    lunaHandler->registerLunaCallback(std::bind(&AutoAndroidDeviceHandler::GetAttachedDeviceStatus, this, _1, _2),
                                                                          GET_DEVICESTATUS);
    lunaHandler->registerLunaCallback(std::bind(&AutoAndroidDeviceHandler::GetAttachedNonStorageDeviceList, this, _1, _2),
                                                                                    GET_NONSTORAGEDEVICELIST);
}

AutoAndroidDeviceHandler::~AutoAndroidDeviceHandler() {
}

bool AutoAndroidDeviceHandler::HandlerEvent(PdmNetlinkEvent* pNE){

    if (pNE->getDevAttribute(ACTION) == "remove")
    {
      ProcessAutoAndroidDevice(pNE);
      if(m_deviceRemoved)
          return true;
    }

    if(isAOAProductId(pNE)) {
        ProcessAutoAndroidDevice(pNE);
        return true;
    }
#ifdef WEBOS_SESSION
    if(isAOAInterface(pNE)) {
        PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d android interface detected", __FUNCTION__,__LINE__);
        if(openDevice(pNE)) {
            PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d opened android device", __FUNCTION__,__LINE__);
            if (startAccessoryMode()>= 0) {
                PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d started Accessory mode", __FUNCTION__,__LINE__);
                return true;
            }
        }
    }
#endif
    return false;
}

bool AutoAndroidDeviceHandler::isAOAProductId(PdmNetlinkEvent* pNE) {
    if (pNE->getDevAttribute(ID_MODEL_ID).find("2d0") != std::string::npos) {
        return true;
    }
    return false;
}

bool AutoAndroidDeviceHandler::openDevice(PdmNetlinkEvent* pNE){
    int usbopen_retrycnt = 0;
    libusb_device_handle *dev_handle = nullptr;
    libusb_device *device = nullptr;
    if(m_context == nullptr) {
        if(int res = libusb_init(&m_context) != LIBUSB_SUCCESS) {
	    PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Fail to init libusb error:%s", __FUNCTION__, __LINE__,libusb_error_name(res));
	    return false;
        } else {
	    PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d libusb_init initialized", __FUNCTION__,__LINE__);
	}
    } else {
        PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d libusb_init already initialized", __FUNCTION__,__LINE__);
    }
    PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d VendorID:%s, ProductID:%s", __FUNCTION__,__LINE__,pNE->getDevAttribute("ID_VENDOR_ID").c_str(),pNE->getDevAttribute("ID_PRODUCT_ID").c_str());
    std::string vendorID = pNE->getDevAttribute("ID_VENDOR_ID").c_str();
    std::string productID =  pNE->getDevAttribute("ID_PRODUCT_ID").c_str();
    unsigned vendorId = stoul(vendorID, nullptr, 16);
    unsigned productId = stoul(productID, nullptr, 16);

    dev_handle = libusb_open_device_with_vid_pid(m_context,vendorId,productId); 
    if(dev_handle == nullptr) {
        PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Fail to get dev_handle", __FUNCTION__, __LINE__);
	return false;
    }

    device = libusb_get_device (dev_handle);
    if(device ==nullptr){
        PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Fail to get device", __FUNCTION__, __LINE__);
	return false;
    }
    while(int res = libusb_open(device, &mHandle) != LIBUSB_SUCCESS) {
        PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to open USB device, error: %s with mHandle : %p", __FUNCTION__, __LINE__, libusb_error_name(res), mHandle);
        usbopen_retrycnt ++;
        if(usbopen_retrycnt > USB_OPEN_RETRY_COUNT) {
            PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to open USB device, exceeded Retry count. (%d)", __FUNCTION__, __LINE__,usbopen_retrycnt);
            return false;
        }
        usleep(USB_OPEN_RETRY_USECOND);
    }
    PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d Successfully opened device. with mHandle : 0x%p", __FUNCTION__,__LINE__,mHandle);

    if (libusb_claim_interface(mHandle, 0) == LIBUSB_SUCCESS) {
        PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d libusb_claim_interface Attached", __FUNCTION__,__LINE__);
    }
    return true;
}

int AutoAndroidDeviceHandler::startAccessoryMode() {
    int ret = -1;
    ret = getAOAPProtocol();
    if(ret <= 0) {
        PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to getAOAPProtocol() (%d)", __FUNCTION__, __LINE__,ret);
        return -1;
    }

    ret = sendString(mHandle, ACCESSORY_MANUFACTURER_NAME, ACCESSORY_STRING_MANUFACTURER);
    if(ret != (static_cast<int>(strlen(ACCESSORY_MANUFACTURER_NAME)) + 1)) {
	PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to Send ACCESSORY_MANUFACTURER_NAME (%d)", __FUNCTION__, __LINE__,ret);
        return -1;
    }

    ret = sendString(mHandle, ACCESSORY_MODEL_NAME, ACCESSORY_STRING_MODEL);
    if(ret != (static_cast<int>(strlen(ACCESSORY_MODEL_NAME)) + 1)) {
	PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to Send ACCESSORY_MODEL_NAME (%d)", __FUNCTION__, __LINE__,ret);
        return -1;
    }

    ret = sendString(mHandle, ACCESSORY_DESCRIPTION, ACCESSORY_STRING_DESCRIPTION);
    if(ret != (static_cast<int>(strlen(ACCESSORY_DESCRIPTION)) + 1)) {
	PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to Send ACCESSORY_DESCRIPTION (%d)", __FUNCTION__, __LINE__,ret);
        return -1;
    }

    ret = sendString(mHandle, ACCESSORY_VERSION, ACCESSORY_STRING_VERSION);
    if(ret != (static_cast<int>(strlen(ACCESSORY_VERSION)) + 1)) {
	PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to Send ACCESSORY_VERSION (%d)", __FUNCTION__, __LINE__,ret);
        return -1;
    }

    ret = sendString(mHandle, ACCESSORY_URI, ACCESSORY_STRING_URI);
    if(ret != (static_cast<int>(strlen(ACCESSORY_URI)) + 1)) {
		PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to Send ACCESSORY_URI (%d)", __FUNCTION__, __LINE__,ret);
        return -1;
    }

    ret = sendString(mHandle, ACCESSORY_SERIAL_NUMBER, ACCESSORY_STRING_SERIAL);
    if(ret != (static_cast<int>(strlen(ACCESSORY_SERIAL_NUMBER)) + 1)) {
        PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to Send ACCESSORY_SERIAL_NUMBER (%d)", __FUNCTION__, __LINE__,ret);
        return -1;
    }

    ret = libusb_control_transfer(mHandle, DEVICE_TO_HOST_TYPE, ACCESSORY_START, 0U, 0U, NULL, 0U, 3000U);
    if(ret != 0) {
	PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to Send ACCESSORY_START (%d)", __FUNCTION__, __LINE__,ret);
        return -1;
    }
    return ret;
}

int AutoAndroidDeviceHandler::getAOAPProtocol() {
    int result = -1;
    uint8_t buffer[2];
    int iAOAPProtoVer = 0;
    int ret = libusb_control_transfer(mHandle, HOST_TO_DEVICE_TYPE, ACCESSORY_GET_PROTOCOL, 0U, 0U,
                                      (uint8_t*) buffer, 2U, 3000U);
    if(ret == 2) {
        iAOAPProtoVer = static_cast<int>(buffer[0]) + static_cast<int>(buffer[1] << 8);
	PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d Succeed to get protocol (ver = %d)", __FUNCTION__,__LINE__,iAOAPProtoVer);
        result = iAOAPProtoVer;
    }
    else {
	PDM_LOG_ERROR("AutoAndroidDeviceHandler:%s line: %d Failed to get protocol with the error: %s", __FUNCTION__, __LINE__,libusb_error_name(ret));
    }
    return result;
}

bool AutoAndroidDeviceHandler::isAOAInterface(PdmNetlinkEvent* pNE)
{
    bool result = false;
    std::string interfaceClass = pNE->getInterfaceClass();
    PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d interfaceClass %s devType:%s", __FUNCTION__,__LINE__,interfaceClass.c_str(), pNE->getDevAttribute(DEVTYPE).c_str());
    if (((interfaceClass.find(":06")!= std::string::npos) || (interfaceClass.find("ff")!= std::string::npos)) && (pNE->getDevAttribute(DEVTYPE) ==  USB_DEVICE) && (pNE->getDevAttribute(ID_BLUETOOTH) != "1")){
        result =  true;
    }
    return result;
}

void AutoAndroidDeviceHandler::removeDevice(AutoAndroidDevice* device)
{
    if(!device)
        return;
    sList.remove(device);
    Notify(AUTO_ANDROID_DEVICE,REMOVE,device);
    delete device;
    device = nullptr;
    if(sList.empty()) {
        if (m_context) {
            PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d list is emplty so stopping the libusb", __FUNCTION__,__LINE__);
            libusb_exit(m_context);
            m_context = nullptr;
	}
    }

}

void AutoAndroidDeviceHandler::ProcessAutoAndroidDevice(PdmNetlinkEvent* pNE){

    PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d DEVTYPE: %s ACTION: %s", __FUNCTION__,__LINE__,pNE->getDevAttribute(DEVTYPE).c_str(),pNE->getDevAttribute(ACTION).c_str());
    AutoAndroidDevice* androidDevice = nullptr;
    try {
        switch(sMapDeviceActions.at(pNE->getDevAttribute(ACTION)))
        {
            case DeviceActions::USB_DEV_ADD:
                androidDevice = getDeviceWithPath< AutoAndroidDevice >(sList,pNE->getDevAttribute(DEVPATH));
                if(!androidDevice)
                {
                    androidDevice = new (std::nothrow) AutoAndroidDevice(m_pConfObj, m_pluginAdapter);
                    if(!androidDevice)
                        break;
                    androidDevice->setDeviceInfo(pNE);
                    androidDevice->registerCallback(std::bind(&AutoAndroidDeviceHandler::commandNotification, this, _1, _2));
                    sList.push_back(androidDevice);
                    Notify(AUTO_ANDROID_DEVICE,ADD, androidDevice);
                }else
                    androidDevice->setDeviceInfo(pNE);
                break;
            case DeviceActions::USB_DEV_REMOVE:
                androidDevice = getDeviceWithPath< AutoAndroidDevice >(sList,pNE->getDevAttribute(DEVPATH));
                if(androidDevice) {
                    removeDevice(androidDevice);
                    m_deviceRemoved = true;
                }
                break;
            default:
                //Do nothing
                break;
        }
    }
    catch (const std::out_of_range& err) {
        PDM_LOG_INFO("AutoAndroidDeviceHandler:",0,"%s line: %d out of range : %s", __FUNCTION__,__LINE__,err.what());
    }
}

bool AutoAndroidDeviceHandler::HandlerCommand(CommandType *cmdtypes, CommandResponse *cmdResponse) {

    PDM_LOG_DEBUG("AutoAndroidDeviceHandler:%s line: %d", __FUNCTION__, __LINE__);
    return false;
}

bool AutoAndroidDeviceHandler::HandlePluginEvent(int eventType)
{
   //Need to discuss
    return true;
}

bool AutoAndroidDeviceHandler::GetAttachedDeviceStatus(pbnjson::JValue &payload, LSMessage *message)
{
    return getAttachedDeviceStatus< AutoAndroidDevice >(sList, payload );
}

bool AutoAndroidDeviceHandler::GetAttachedNonStorageDeviceList(pbnjson::JValue &payload, LSMessage *message)
{
   return getAttachedNonStorageDeviceList< AutoAndroidDevice >( sList, payload );
}

void AutoAndroidDeviceHandler::commandNotification(EventType event, AutoAndroidDevice* device)
{

    Notify(AUTO_ANDROID_DEVICE,event,device);

}
