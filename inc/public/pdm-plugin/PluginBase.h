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

#ifndef PLUGINBASE_H_
#define PLUGINBASE_H_

#include "Api.h"
#include <lunaservice.h>

#include "IDevice.h"

class PluginBase: public Plugin {

protected:
    Mananger *m_Manager;
    LSHandle *m_lunaHandle;

public:

    PluginBase(Mananger *manager, LSHandle *lunaHandle):
                m_Manager(manager), m_lunaHandle(lunaHandle) {}

    virtual ~PluginBase() {
        m_Manager = nullptr;
        m_lunaHandle = nullptr;
    }

    virtual bool init() {
        return true;
    }

    virtual void deInit() {
        return;
    }

    virtual int getDeviceNumber(IDevice &device) const {
        return 0;
    }

    virtual bool isUmoutBlocked(const std::string &path) {
        return true;
    }

    virtual void notifyChange(int eventID, int eventType, IDevice* device) {
        return;
    }

};
#endif /* PLUGINBASE_H_ */
