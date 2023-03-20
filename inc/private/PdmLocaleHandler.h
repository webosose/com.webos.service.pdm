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

#ifndef _PDMLOCALEHANDLER_H
#define _PDMLOCALEHANDLER_H

#include <luna-service2/lunaservice.h>
#include <webosi18n.h>

class PdmLocaleHandler
{
private:
    LSHandle *m_Handle;
    std::string m_locale_info_;
    ResBundle* m_resBundle;

    PdmLocaleHandler();
    LSHandle *getHandle();
    bool registerSettingsServiceServerState(void);
    void setLocale(const std::string& locale);

public:
    ~PdmLocaleHandler();
    PdmLocaleHandler(const PdmLocaleHandler& src) = delete;
    PdmLocaleHandler& operator=(const PdmLocaleHandler& rhs) = delete;
    static PdmLocaleHandler *getInstance();
    static bool onSettingsServiceStateChanged(LSHandle * sh, LSMessage * message,void * user_data);
    static bool onLocaleInfoReceived(LSHandle * sh, LSMessage * message,void * user_data);
    bool init();
    std::string getLocString(const std::string& key);
};

#endif //_PDMLOCALEHANDLER_H
