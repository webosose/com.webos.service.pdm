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

#include "DeviceStateObserver.h"

void DeviceStateObserver::Register(IObserver* observer) {
    if(observer)
        _observers->push_front(observer);
}

void DeviceStateObserver::Unregister(IObserver* observer) {

    if(observer)
        _observers->remove(observer);

}

void DeviceStateObserver::Notify(const int &eventDeviceType, const int &eventID, IDevice* device) {
    for (std::list<IObserver*>::iterator it = _observers->begin(); it != _observers->end(); it++)
        (*it)->update(eventDeviceType,eventID,device);
}

DeviceStateObserver::DeviceStateObserver() {

    _observers = new std::list<IObserver*>();

}

DeviceStateObserver::DeviceStateObserver(const DeviceStateObserver& obj) {
    _observers = new std::list<IObserver*>();
    *_observers = *obj._observers;
}

DeviceStateObserver& DeviceStateObserver::operator=(const DeviceStateObserver& obj) {
    if ( this != &obj) {
        *_observers = *obj._observers;
    }
    return *this;
}

DeviceStateObserver::~DeviceStateObserver() {
    _observers->clear();
    delete _observers;
    _observers = nullptr;
 }
