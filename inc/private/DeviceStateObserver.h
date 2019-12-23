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

#ifndef DEVICESTATEOBSERVER_H_
#define DEVICESTATEOBSERVER_H_

#include <list>
#include "IObserver.h"

class DeviceStateObserver {
public:
    virtual ~DeviceStateObserver();
    virtual void Register(IObserver* observer);
    virtual void Unregister(IObserver* observer);
protected:
    DeviceStateObserver();
    DeviceStateObserver(const DeviceStateObserver& obj);
    DeviceStateObserver& operator=(const DeviceStateObserver& obj);
    void Notify(const int &eventDeviceType, const int &eventID, IDevice* device = nullptr);
private:
    std::list<IObserver*> *_observers;
};

#endif /* DEVICESTATEOBSERVER_H_ */
