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

#include "PdmNotification.h"
#include "DeviceHandler.h"
#include "PdmLogUtils.h"
#include "DeviceManager.h"
#include "LunaIPC.h"
#include "Common.h"

bool Notification::mNotifyMgrEnable = true;

// Active alerts
static std::unordered_map<std::string, std::string> alerts;
static std::string internalId;

Notification::Notification()
            : mHandle(nullptr)
{

}

Notification::~Notification()
{

}

bool Notification::init()
{
    mHandle = LunaIPC::getInstance()->getLSHandle();
    return registerNotificationServerState();
}

LSHandle * Notification::getHandle()
{
    return mHandle;
}

bool Notification::createToast(
    const std::string &message,
    const std::string &iconUrl,
    const pbnjson::JValue &onClickAction)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool retValue = false;
    if (mNotifyMgrEnable == false) {
        PDM_LOG_ERROR("Notification: %s line : %d NotificationMgr service is not running",__FUNCTION__, __LINE__);
        return false;
    }

    pbnjson::JObject params = pbnjson::JObject();
    params.put("message", pbnjson::JValue(message));
    params.put("sourceId", "com.webos.service.pdm");

    if (iconUrl.length() > 0)
    {
        params.put("iconUrl", pbnjson::JValue(iconUrl));
    }

    if (!onClickAction.isNull())
    {
        params.put("onclick", onClickAction);
    }

    retValue = LSCallOneReply( getHandle(),
        "luna://com.webos.notification/createToast",
        params.stringify().c_str(),
        NULL, NULL, NULL, &lserror);

    if(!retValue)
    {
        PDM_LOG_ERROR("Notification: %s line : %d error on LSCallOneReply for createToast",__FUNCTION__, __LINE__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return retValue;
}
bool Notification::createAlert(const std::string &alertId,
                                const std::string &message,
                                const pbnjson::JValue &buttons,
                                const std::string &title,
                                const std::string &iconUrl,
                                bool modal,
                                const pbnjson::JValue &onClose)
{
    PDM_LOG_DEBUG("Notification: %s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    bool retValue = false;

    if (mNotifyMgrEnable == false) {
       PDM_LOG_ERROR("Notification: %s line : %d NotificationMgr service is not running",__FUNCTION__, __LINE__);
        return false;
    }

    (void) this->closeAlert(alertId);
    internalId = alertId;

    pbnjson::JObject params = pbnjson::JObject{{"title", pbnjson::JValue(title)},
            {"modal", pbnjson::JValue(modal)},
            {"message", pbnjson::JValue(message)},
            {"buttons", buttons}};

    if (!onClose.isNull())
    {
        params.put("onclose", onClose);
    }

    if (iconUrl.length() > 0)
    {
        params.put("iconUrl", pbnjson::JValue(iconUrl));
    }

    retValue = LSCallOneReply( getHandle(),
        "luna://com.webos.notification/createAlert",
        params.stringify().c_str(),
        createAlertCallback, NULL, NULL, &lserror);

    if(!retValue)
    {
        PDM_LOG_ERROR("Notification: %s line : %d error on LSCallOneReply for createAlert",__FUNCTION__, __LINE__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return retValue;
}

bool Notification::createAlertCallback(LSHandle * sh, LSMessage * message, void* ctx)
{
    PDM_LOG_DEBUG("Notification: %s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    bool retValue = false;

    LSMessageRef(message);
    const char *payload = LSMessageGetPayload(message);

    pbnjson::JValue root = pbnjson::JDomParser::fromString(payload);

    if(root.isNull()) {
        PDM_LOG_CRITICAL("Notification:%s line: %d json object error", __FUNCTION__, __LINE__);
        LSMessageUnref(message);
        return false;
    }
    std::string internalIdString = "";

    if (root["alertId"].isString()) {
        if (root["alertId"].asString(internalIdString) == CONV_OK) {
            PDM_LOG_DEBUG("Notification: %s line: %d internalIdString: %s", __FUNCTION__, __LINE__,internalIdString.c_str());
            alerts[internalId] = internalIdString;
        }
        else
            PDM_LOG_ERROR("Notification: %s line : %d asString error",__FUNCTION__, __LINE__);
    }

    if(root["returnValue"].isBoolean()) {
        retValue = root["returnValue"].asBool();
    }

    if(!retValue)
    {
       PDM_LOG_ERROR("Notification: %s line : %d error on payload message",__FUNCTION__, __LINE__);
       LSErrorPrint(&lserror, stderr);
       LSErrorFree(&lserror);
    }

    LSMessageUnref(message);
    return retValue;
}

bool Notification::closeAlert(const std::string &alertId)
{
    PDM_LOG_DEBUG("Notification: %s line: %d", __FUNCTION__, __LINE__);
    LSError lserror;
    LSErrorInit(&lserror);
    bool retValue = false;

    if (mNotifyMgrEnable == false) {
        PDM_LOG_ERROR("Notification: %s line : %d NotificationMgr service is not running",__FUNCTION__, __LINE__);
        return false;
    }
    if (alerts.count(alertId) == 0)
    {
        PDM_LOG_ERROR("Notification: %s line : %d No open alerts",__FUNCTION__, __LINE__);
        return false;
    }

    pbnjson::JObject params = pbnjson::JObject{{"alertId", pbnjson::JValue(alerts[alertId])}};
    alerts.erase(alertId);
    retValue = LSCallOneReply( getHandle(),
        "luna://com.webos.notification/closeAlert",
        params.stringify().c_str(),
        NULL, NULL, NULL, &lserror);

    if(!retValue)
    {
        PDM_LOG_ERROR("Notification: %s line : %d error on LSCallOneReply for closeAlert",__FUNCTION__, __LINE__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return retValue;
}

bool Notification::registerNotificationServerState(void)
{
    PDM_LOG_DEBUG("Notification: %s line: %d", __FUNCTION__, __LINE__);
    bool retValue = false;
    LSError lserror;
    LSErrorInit(&lserror);

    pbnjson::JValue Obj = pbnjson::Object();
    Obj.put("subscribe", true);
    Obj.put("serviceName", "com.webos.notification");

    retValue = LSCall( getHandle(),
        "luna://com.webos.service.bus/signal/registerServerStatus",
        Obj.stringify().c_str(),
        getNotificationStateCallback, NULL, NULL, &lserror);

    if(!retValue)
    {
        PDM_LOG_ERROR("Notification: %s line : %d error on LSCall for registerNotificationServerState",__FUNCTION__, __LINE__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
    return retValue;
}

bool Notification::getNotificationStateCallback(LSHandle * sh, LSMessage * message,void * ctx)
{
    PDM_LOG_DEBUG("Notification: %s line: %d", __FUNCTION__, __LINE__);
    const char *payload;
    LSError lserror;
    LSErrorInit(&lserror);

    LSMessageRef(message);

    payload = LSMessageGetPayload(message);
    pbnjson::JValue root = pbnjson::JDomParser::fromString(payload);

    if(root.isNull()) {
        PDM_LOG_ERROR("Notification:%s line: %d json object error", __FUNCTION__, __LINE__);
        LSMessageUnref(message);
        return false;
    }

    std::string serviceName = root["serviceName"].asString();

    if(root["connected"].isBoolean()) {
        mNotifyMgrEnable = root["connected"].asBool();
    }
    PDM_LOG_INFO("Notification:",0,"%s line: %dNotificationMgr is connected =%d", __FUNCTION__,__LINE__,mNotifyMgrEnable);
    LSMessageUnref(message);

    return true;
}
