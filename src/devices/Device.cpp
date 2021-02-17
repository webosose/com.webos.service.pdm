// Copyright (c) 2019-2021 LG Electronics, Inc.
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

#include <cstddef>
#include <fstream>
#include <luna-service2/lunaservice.hpp>
#include <luna-service2++/handle.hpp>

#include "Device.h"
#include "PdmLogUtils.h"
#include "LunaIPC.h"

#define USB_SYS_USBDEVICE_PATH                        "/sys/bus/usb/devices/"

using namespace PdmDevAttributes;

Device::Device(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter, std::string deviceType, std::string errorReason):
    m_isPowerOnConnect(false), m_isToastRequired(true), m_deviceNum(0), m_usbPortNum(0), m_busNum(0), m_usbPortSpeed(0), m_pConfObj(pConfObj)
    , m_pluginAdapter(pluginAdapter), m_deviceStatus(PdmDevAttributes::PDM_ERR_NOTHING), m_deviceType(deviceType), m_errorReason(errorReason), m_deviceName("")
    , m_devicePath(""), m_serialNumber(""), m_vendorName(""), m_productName(""), m_devSpeed(""), m_deviceSubType("")
#ifdef WEBOS_SESSION
    , m_devPath(""), m_vendorID(""), m_productID(""), m_hubPortNumber(""), m_deviceSetId("")
#endif
{
}

#ifdef WEBOS_SESSION
bool Device::readFromFile(std::string fileToRead, std::string &usbData)
{
    std::ifstream readFile (fileToRead);
    if (readFile.fail())
        return false;
    readFile >> usbData;
    readFile.close();
    return true;
}

void Device::getBasicUsbInfo(std::string devPath)
{
    std::string devFullPath = USB_SYS_USBDEVICE_PATH + devPath;

    // Read idVendor
    std::string vendorIdFile = devFullPath + "/idVendor";
    if(!readFromFile(vendorIdFile, m_vendorID))
        return;

    // Read idProduct
    std::string productIdFile = devFullPath + "/idProduct";
    if(!readFromFile(productIdFile, m_productID))
        return;
}
#endif

void Device::setDeviceInfo(PdmNetlinkEvent* pNE)
{
    m_serialNumber = pNE->getDevAttribute(ID_SERIAL_SHORT);
    m_deviceSubType = pNE->getDevAttribute(ID_USB_DRIVER);
    m_productName = pNE->getDevAttribute(ID_MODEL);

#ifdef WEBOS_SESSION
    if(pNE->getDevAttribute(DEVTYPE) == USB_DEVICE)
        m_hubPortNumber = pNE->getDevAttribute(USB_PORT);
#endif

    if (!m_pluginAdapter->getPowerState() || pNE->getDevAttribute(IS_POWER_ON_CONNECT) == "true")
      m_isPowerOnConnect = true;

    if(pNE->getDevAttribute(DEVTYPE) == USB_DEVICE){
      m_devicePath = pNE->getDevAttribute(DEVPATH);
#ifdef WEBOS_SESSION
      std::string delimeter = "/";
      std::string usbInfoString = m_devicePath.substr(m_devicePath.rfind(delimeter) + delimeter.size());
      getBasicUsbInfo(usbInfoString);
#endif
    }
    if(!pNE->getDevAttribute(ID_VENDOR_FROM_DATABASE).empty()){
        m_vendorName = pNE->getDevAttribute(ID_VENDOR_FROM_DATABASE);
    } else {
        m_vendorName = pNE->getDevAttribute(ID_VENDOR);
    }
    if(!pNE->getDevAttribute(DEVNUM).empty())
        m_deviceNum = std::stoi(pNE->getDevAttribute(DEVNUM),nullptr);

#ifdef WEBOS_SESSION
    if (!pNE->getDevAttribute(DEVNAME).empty()) {
        std::string devPath = "/dev/";
        m_devPath = devPath.append(pNE->getDevAttribute(DEVNAME));
    }
#endif
}

#ifdef WEBOS_SESSION
void Device::setDeviceSetId(std::string hubPortPath)
{
    PDM_LOG_DEBUG("Device::%s line:%d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);

    m_deviceSetId = "";

    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue query;

    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                             {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};

    find_query.put("query", query);

    LS::Payload find_payload(find_query);
    LS::Call call = LunaIPC::getInstance()->getLSCPPHandle()->callOneReply("luna://com.webos.service.db/find", find_payload.getJson(), NULL, this, NULL);
    LS::Message message = call.get();

    LS::PayloadRef response_payload = message.accessPayload();
    pbnjson::JValue request = response_payload.getJValue();

    if(request.isNull())
    {
        PDM_LOG_DEBUG("Db8 LS2 response is empty in %s", __PRETTY_FUNCTION__ );
    }

    if(!request["returnValue"].asBool())
    {
        PDM_LOG_DEBUG("Call to Db8 to save/delete message failed in %s", __PRETTY_FUNCTION__ );
    }

    m_deviceSetId = request["results"][0]["deviceSetId"].asString();

    PDM_LOG_DEBUG("Device::%s line:%d deviceSetId: %s", __FUNCTION__, __LINE__, m_deviceSetId.c_str());
}

std::string Device::getErrorReason(std::string hubPortPath)
{
    PDM_LOG_DEBUG("Device::%s line:%d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);

    std::string m_errorReason;

    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue query;

    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                             {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};

    find_query.put("query", query);

    LS::Payload find_payload(find_query);
    LS::Call call = LunaIPC::getInstance()->getLSCPPHandle()->callOneReply("luna://com.webos.service.db/find", find_payload.getJson(), NULL, this, NULL);
    LS::Message message = call.get();

    LS::PayloadRef response_payload = message.accessPayload();
    pbnjson::JValue request = response_payload.getJValue();

    if(request.isNull())
    {
        PDM_LOG_DEBUG("Db8 LS2 response is empty in %s", __PRETTY_FUNCTION__ );
    }

    if(!request["returnValue"].asBool())
    {
        PDM_LOG_DEBUG("Call to Db8 to is failed in %s", __PRETTY_FUNCTION__ );
    }

    m_errorReason = request["results"][0]["errorReason"].asString();
    PDM_LOG_DEBUG("Device::%s line:%d deviceSetId: %s", __FUNCTION__, __LINE__, m_errorReason.c_str());
    if(m_errorReason.empty()) {
        m_errorReason = "Not selected";
    }
    return m_errorReason;
}

std::string Device::getStorageRootPath(std::string hubPortPath)
{
    PDM_LOG_DEBUG("Device::%s line:%d hubPortPath:%s", __FUNCTION__, __LINE__, hubPortPath.c_str());
    LSError lserror;
    LSErrorInit(&lserror);

    std::string m_rootPath;

    pbnjson::JValue find_query = pbnjson::Object();
    pbnjson::JValue query;

    query = pbnjson::JObject{{"from", "com.webos.service.pdmhistory:1"},
                             {"where", pbnjson::JArray{{{"prop", "hubPortPath"}, {"op", "="}, {"val", hubPortPath.c_str()}}}}};

    find_query.put("query", query);

    LS::Payload find_payload(find_query);
    LS::Call call = LunaIPC::getInstance()->getLSCPPHandle()->callOneReply("luna://com.webos.service.db/find", find_payload.getJson(), NULL, this, NULL);
    LS::Message message = call.get();

    LS::PayloadRef response_payload = message.accessPayload();
    pbnjson::JValue request = response_payload.getJValue();

    if(request.isNull())
    {
        PDM_LOG_DEBUG("Db8 LS2 response is empty in %s", __PRETTY_FUNCTION__ );
    }

    if(!request["returnValue"].asBool())
    {
        PDM_LOG_DEBUG("Call to Db8 to is failed in %s", __PRETTY_FUNCTION__ );
    }

    m_rootPath = request["results"][0]["rootPath"].asString();
    PDM_LOG_DEBUG("Device::%s line:%d deviceSetId: %s", __FUNCTION__, __LINE__, m_rootPath.c_str());
    if(m_rootPath.empty()) {
        m_rootPath = "Device not selected";
    }
    return m_rootPath;
}
#endif
std::string  Device::getDeviceSpeed(int speed) const {

    switch(speed){
        case SUPER:
            return USB_SUPER_SPEED;
        case HIGH:
            return USB_HIGH_SPEED;
        case FULL:
            return USB_FULL_SPEED;
        default:
            return USB_LOW_SPEED;
    }
}
