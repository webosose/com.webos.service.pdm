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

#ifndef _NOTIFICATION_MGR_H_
#define _NOTIFICATION_MGR_H_

#include <lunaservice.h>
#include <string>
#include <unordered_map>
#include "PdmNotificationInterface.h"
#include "pbnjson.hpp"

class Notification :public PdmNotificationInterface
{
    public:
        Notification();
        ~Notification();

        bool init();
        LSHandle *getHandle();
        bool registerNotificationServerState(void);
        static bool getNotificationStateCallback(LSHandle * sh, LSMessage * message,void * ctx);
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
        static bool createAlertCallback(LSHandle * sh, LSMessage * message, void* ctx);

    private:
        LSHandle *mHandle;
        static bool mNotifyMgrEnable;
};

#endif /* _NOTIFICATION_MGR_H_ */
