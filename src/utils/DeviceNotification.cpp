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

#include "DeviceNotification.h"
#include "PdmNotification.h"
#include "PdmLogUtils.h"

DeviceNotification *DeviceNotification::getInstance() {
    static DeviceNotification _instance;
    return &_instance;
}

DeviceNotification::DeviceNotification():mPdmNotificationInterface(nullptr)
{

}

DeviceNotification::~DeviceNotification()
{

}

bool DeviceNotification::init()
{
    PDM_LOG_DEBUG("DeviceNotification: %s line: %d", __FUNCTION__, __LINE__);
    mPdmNotificationInterface = nullptr;
    return setNotifier();
}

bool DeviceNotification::deInit()
{
    PDM_LOG_DEBUG("DeviceNotification: %s line: %d", __FUNCTION__, __LINE__);
    delete mPdmNotificationInterface;
    mPdmNotificationInterface = nullptr;
    return true;
}

bool DeviceNotification::createToast(
    const std::string &message,
    const std::string &iconUrl,
    const pbnjson::JValue &onClickAction)
{
    PDM_LOG_DEBUG("DeviceNotification: %s line: %d", __FUNCTION__, __LINE__);
    return mPdmNotificationInterface->createToast(message,iconUrl,onClickAction);
}

bool DeviceNotification::createAlert(const std::string &alertId,
    const std::string &message,
    const pbnjson::JValue &buttons,
    const std::string &title,
    const std::string &iconUrl,
    bool modal,
    const pbnjson::JValue &onClose)
{
    PDM_LOG_DEBUG("DeviceNotification: %s line: %d", __FUNCTION__, __LINE__);
    return mPdmNotificationInterface->createAlert(alertId,message,buttons,title,iconUrl,modal,onClose);
}

bool DeviceNotification::closeAlert(const std::string &alertId)
{
    PDM_LOG_DEBUG("DeviceNotification: %s line: %d", __FUNCTION__, __LINE__);
    return mPdmNotificationInterface->closeAlert(alertId);
}

//Todo: setNotifier can take arguments to set based on a config file
bool DeviceNotification::setNotifier()
{
    PDM_LOG_DEBUG("DeviceNotification: %s line: %d", __FUNCTION__, __LINE__);
    mPdmNotificationInterface = new(std::nothrow) Notification();
    if(mPdmNotificationInterface == nullptr)
    {
        PDM_LOG_ERROR("DeviceNotification: %s line: %d Failed allocation for Notification", __FUNCTION__, __LINE__);
        return false;
    }
    return mPdmNotificationInterface->init();
}
