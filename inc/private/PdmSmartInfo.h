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

#ifndef _PDMSMARTINFO_H
#define _PDMSMARTINFO_H

#include <string>
#include <map>
#include <list>
#include <vector>

class PdmSmartInfo {
private:
    std::string m_deviceName;
    std::map<std::string,std::string> m_smartdeviceInfoMap;
    std::vector<std::vector<std::string>> m_attributeInfo;
    void pdmSmartGetHealth();
    void pdmSmartGetDeviceInfo();
    void pdmSmartLogDevInfo();
    void pdmSmartGetAttributes();
    void pdmSmartLogAttributeInfo();

    const std::list<std::string> importantDevInfo = {
           "Vendor",
           "Model Family",
           "Product",
           "Device Model",
           "User Capacity",
           "SMART support is"
    };

    const std::vector<std::string> attributeHeaders = {
           "ID",
           "ATTRIBUTE_NAME",
           "FLAG",
           "VALUE",
           "WORST",
           "THRESH",
           "TYPE",
           "UPDATED",
           "WHEN_FAILED",
           "RAW_VALUE"
    };

    const std::list<std::string> importantAttributeInfoId = {
           "1",
           "5",
           "7",
           "10",
           "184",
           "187",
           "188",
           "196",
           "197",
           "198",
           "199",
           "201"
    };

public:

    PdmSmartInfo() = default;
    ~PdmSmartInfo();
    void setDeviceName(std::string deviceName);
    void logPdmSmartInfo();
};

#endif
