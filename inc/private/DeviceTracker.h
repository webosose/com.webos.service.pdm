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

#ifndef __DEVICE_TRACKER_H_
#define __DEVICE_TRACKER_H_

#include <functional>
#include "IObserver.h"

using lunaCb = std::function<bool(int)>;

class PluginAdapter;

class DeviceTracker :public IObserver
{
    private :
        PluginAdapter* m_pluginAdapter;
        void update(const int &eventDeviceType, const int &eventID, IDevice* device = nullptr) override;
        lunaCb mLunaServiceCallback;

    public :
        void attachObservers();
        DeviceTracker(PluginAdapter* const pluginAdapter);
        ~DeviceTracker();
        void RegisterCallback(lunaCb);
};

#endif //__DEVICE_TRACKER_H_