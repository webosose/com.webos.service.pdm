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

#ifndef _DEVICE_NOTIFICATION_H_
#define _DEVICE_NOTIFICATION_H_

#include <string>
#include "pbnjson.hpp"

class PdmNotificationInterface;
class DeviceNotification
{
private:
    DeviceNotification();
    DeviceNotification(const DeviceNotification& src) = delete;
    DeviceNotification& operator=(const DeviceNotification& rhs) = delete;
    PdmNotificationInterface *mPdmNotificationInterface;

public:
    static DeviceNotification *getInstance();
    bool createToast(
        const std::string &message,
        const std::string &iconUrl = std::string(),
        const pbnjson::JValue &onClickAction = pbnjson::JValue());

    bool createAlert(const std::string &alertId,
        const std::string &message,
        const pbnjson::JValue &buttons,
        const std::string &title = std::string(),
        const std::string &iconUrl = std::string(),
        bool modal = false,
        const pbnjson::JValue &onClose  = pbnjson::JValue());

    bool closeAlert(const std::string &alertId);
    bool setNotifier();
    ~DeviceNotification();
    bool init();
    bool deInit();
};


#endif /* _DEVICE_NOTIFICATION_H_ */
