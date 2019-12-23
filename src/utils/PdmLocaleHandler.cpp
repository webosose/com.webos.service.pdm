// Copyright (c) 2019 LG Electronics, Inc.
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

#include "PdmLocaleHandler.h"
#include "PdmLogUtils.h"
#include "LunaIPC.h"

PdmLocaleHandler::PdmLocaleHandler()
    : m_Handle(nullptr)
    , m_locale_info_("en-US")
    , m_resBundle(nullptr)
{
}

PdmLocaleHandler::~PdmLocaleHandler()
{
    if(m_resBundle)
    {
        delete m_resBundle;
        m_resBundle = nullptr;
    }
}

PdmLocaleHandler *PdmLocaleHandler::getInstance() {
    static PdmLocaleHandler _instance;
    return &_instance;
}

bool PdmLocaleHandler::init()
{
    std::string locale = "en-US";
    const std::string file = "cppstrings.json";
    const std::string resources_path = "/usr/share/localization/@CMAKE_PROJECT_NAME@";
    m_resBundle = new (std::nothrow) ResBundle(locale, file, resources_path);
    if(!m_resBundle)
    {
        PDM_LOG_ERROR("PdmLocaleHandler: %s line: %d Failed allocation for ResBundle", __FUNCTION__, __LINE__);
        return false;
    }
    m_Handle = LunaIPC::getInstance()->getLSHandle();
    return registerSettingsServiceServerState();
}

LSHandle * PdmLocaleHandler::getHandle()
{
    PDM_LOG_DEBUG("PdmLocaleHandler: %s line: %d m_Handle =%p", __FUNCTION__, __LINE__,m_Handle);
    return m_Handle;
}

bool PdmLocaleHandler::registerSettingsServiceServerState(void)
{
    PDM_LOG_DEBUG("PdmLocaleHandler: %s line: %d", __FUNCTION__, __LINE__);
    bool retValue = false;
    LSError lserror;
    LSErrorInit(&lserror);
    std::string payload = "{\"subscribe\":true,\"serviceName\":\"com.webos.settingsservice\"}";
    retValue = LSCall( getHandle(),
        "luna://com.webos.service.bus/signal/registerServerStatus",
        payload.c_str(),
        onSettingsServiceStateChanged, NULL, NULL, &lserror);
    if(!retValue)
    {
        PDM_LOG_ERROR("PdmLocaleHandler: %s line : %d error on LSCall for registerSettingsServiceServerState",__FUNCTION__, __LINE__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
        return retValue;
    }

    retValue = LSCall( getHandle(),
       "luna://com.webos.settingsservice/getSystemSettings",
       R"({"keys":["localeInfo"],"subscribe":true})",
       onLocaleInfoReceived, NULL, NULL, &lserror);
   if(!retValue)
   {
       PDM_LOG_ERROR("PdmLocaleHandler: %s line : %d error on LSCall for getSystemSettings",__FUNCTION__, __LINE__);
       LSErrorPrint(&lserror, stderr);
       LSErrorFree(&lserror);
   }
    return retValue;
}

bool PdmLocaleHandler::onSettingsServiceStateChanged(LSHandle * sh, LSMessage * message, void * user_data)
{
    PDM_LOG_DEBUG("PdmLocaleHandler: %s line: %d", __FUNCTION__, __LINE__);
    bool retValue = false;
    LSError lserror;
    LSErrorInit(&lserror);
    LSMessageRef(message);
    const char *msgPayload;
    msgPayload = LSMessageGetPayload(message);
    pbnjson::JValue root = pbnjson::JDomParser::fromString(msgPayload);
    if(root.isNull()) {
        PDM_LOG_ERROR("PdmLocaleHandler:%s line: %d json object error", __FUNCTION__, __LINE__);
        LSMessageUnref(message);
        return retValue;
    }
    if(root["connected"].isBoolean()) {
        if(true == root["connected"].asBool())
        {
            retValue = LSCall( PdmLocaleHandler::getInstance()->getHandle(),
                "luna://com.webos.settingsservice/getSystemSettings",
                R"({"keys":["localeInfo"],"subscribe":true})",
                onLocaleInfoReceived, NULL, NULL, &lserror);
            if(!retValue)
            {
                PDM_LOG_ERROR("PdmLocaleHandler: %s line : %d error on LSCall for getSystemSettings",__FUNCTION__, __LINE__);
                LSErrorPrint(&lserror, stderr);
                LSErrorFree(&lserror);
            }
        }
    }
    LSMessageUnref(message);
    return retValue;
}

bool PdmLocaleHandler::onLocaleInfoReceived(LSHandle * sh, LSMessage * message, void * user_data)
{
    PDM_LOG_DEBUG("PdmLocaleHandler: %s line: %d", __FUNCTION__, __LINE__);

    LSMessageRef(message);
    const char *msgPayload;
    msgPayload = LSMessageGetPayload(message);
    pbnjson::JValue root = pbnjson::JDomParser::fromString(msgPayload);
    if(root.isNull() || !root["returnValue"].asBool() || !root["settings"].isObject()) {
        PDM_LOG_ERROR("PdmLocaleHandler:%s line: %d json object error", __FUNCTION__, __LINE__);
        LSMessageUnref(message);
        return false;
    }
    std::string ui_locale_info;
    const pbnjson::JValue& j_locale = root["settings"];
    if (j_locale.hasKey("localeInfo") && j_locale["localeInfo"].hasKey("locales") && j_locale["localeInfo"]["locales"].hasKey("UI")) {
        ui_locale_info = j_locale["localeInfo"]["locales"]["UI"].asString();
    }
    if (!ui_locale_info.empty() && ui_locale_info != PdmLocaleHandler::getInstance()->m_locale_info_)
    {
        PdmLocaleHandler::getInstance()->m_locale_info_ = ui_locale_info;
        PdmLocaleHandler::getInstance()->setLocale(ui_locale_info);
    }

    LSMessageUnref(message);
    return true;
}

std::string PdmLocaleHandler::getLocString(const std::string& key)
{
    return m_resBundle->getLocString(key);
}

void PdmLocaleHandler::setLocale(const std::string& locale)
{
    if(locale.empty())
    {
        PDM_LOG_DEBUG("PdmLocaleHandler: %s line: %d locale is empty", __FUNCTION__, __LINE__);
        return;
    }
    if(m_resBundle)
    {
        delete m_resBundle;
        m_resBundle = nullptr;
    }

    std::string newLocale = locale;
    const std::string file = "cppstrings.json";
    const std::string resources_path = "/usr/share/localization/@CMAKE_PROJECT_NAME@";
    m_resBundle = new (std::nothrow) ResBundle(newLocale, file, resources_path);
    if(!m_resBundle)
        PDM_LOG_ERROR("PdmLocaleHandler: %s line: %d Failed allocation for ResBundle", __FUNCTION__, __LINE__);

    PDM_LOG_DEBUG("PdmLocaleHandler: %s line: %d Locale is set as %s", __FUNCTION__, __LINE__, locale.c_str());
}
