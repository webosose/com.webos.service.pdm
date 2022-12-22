// Copyright (c) 2019-2022 LG Electronics, Inc.
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

#ifndef __PDM_NOTIFICATION_MANAGER_H_
#define __PDM_NOTIFICATION_MANAGER_H_

#include "IObserver.h"
#include "DeviceNotification.h"
#include "PdmLocaleHandler.h"

class PdmNotificationManager :public IObserver
{
    private:
        bool m_powerState;
        PdmLocaleHandler *m_pLocHandler;

    private :
        void update(const int &eventDeviceType, const int &eventID, IDevice* device = nullptr) override;
        void showToast(const std::string& message,const std::string &iconUrl);
        bool isToastRequired(int eventDeviceType);
        void showConnectingToast(int eventDeviceType);
        void showFormatStartedToast(IDevice* device);
        void showFormatSuccessToast(IDevice* device);
        void showFormatFailToast(IDevice* device);

        bool createToast(
            const std::string &message,
            const std::string &iconUrl = std::string(),
            const pbnjson::JValue &onClickAction = pbnjson::JValue())
            {
                return DeviceNotification::getInstance()->createToast(message,iconUrl,onClickAction);
            }

        bool createAlert(const std::string &alertId,
            const std::string &message,
            const pbnjson::JValue &buttons,
            const std::string &title = std::string(),
            const std::string &iconUrl = std::string(),
            bool modal = false,
            const pbnjson::JValue &onClose  = pbnjson::JValue())
            {
                return DeviceNotification::getInstance()->createAlert(alertId,message,buttons,title,iconUrl,modal,onClose);
            }

        bool closeAlert(const std::string &alertId)
        {
            return DeviceNotification::getInstance()->closeAlert(alertId);
        }

        void createAlertForMaxUsbStorageDevices();
        void unMountMtpDeviceAlert(IDevice* device);
        void createAlertForFsckTimeout(IDevice* device);
        void createAlertForUnmountedDeviceRemoval(IDevice* device);
        void createAlertForUnsupportedFileSystem(IDevice* device);
        void closeUnsupportedFsAlert(IDevice* device);

    public :
        void attachObservers();
        PdmNotificationManager();
        ~PdmNotificationManager();
        bool HandlePluginEvent(int eventType);
};

#endif //__PDM_NOTIFICATION_MANAGER_H_
