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

#include "PdmLogUtils.h"
#include "PdmSmartInfo.h"
#include "PdmUtils.h"
#include <sstream>
#include <algorithm>
#include <iterator>

void PdmSmartInfo::setDeviceName(std::string deviceName)
{
    m_deviceName  = "/dev/" + deviceName;
}

PdmSmartInfo::~PdmSmartInfo()
{
    std::vector<std::vector<std::string>>::iterator itAttrInfo;
    // Loop through and clear the vectors one by one inside m_attributeInfo
    for(itAttrInfo = m_attributeInfo.begin(); itAttrInfo != m_attributeInfo.end(); itAttrInfo++) {
        (*itAttrInfo).clear();
    }
    //Finally clear the main vector m_attributeInfo]
    m_attributeInfo.clear();
    m_smartdeviceInfoMap.clear();
}
void PdmSmartInfo::pdmSmartGetHealth()
{
    std::string sysCommand = "smartctl -H -d sat -T permissive " +  m_deviceName + " | grep \"overall-health\"";
    std::string result = PdmUtils::execShellCmd(sysCommand);

    if(!result.empty()) {
        std::string resultPhrase = "\"SMART overall-health self-assessment test result:\"";
        if(std::string::npos == result.find("PASSED")) {
            resultPhrase = resultPhrase + " \"FAILED\"";
        }else{
            sysCommand = "smartctl -A -i --log=error -d sat -T permissive " + m_deviceName + " |egrep \"Error Count\"";
            result = PdmUtils::execShellCmd(sysCommand);
            if(!result.empty())
                resultPhrase = resultPhrase + " \"WARNING\"" + " \"Error Count\":\" \"" + result + "\"";
            else
                resultPhrase = resultPhrase + " \"PASSED\"" + " \"Error Count\":\"No_Errors_Logged\"";
        }
     PDM_LOG_INFO("PDM_SMART:",0,"%s", resultPhrase.c_str());
    }
}

void PdmSmartInfo::pdmSmartGetDeviceInfo()
{
    std::string sysCommand = "smartctl -i -d sat,auto -T permissive " +  m_deviceName;
    std::string result = PdmUtils::execShellCmd(sysCommand);

    if(!result.empty()) {
        int pos = result.find_last_of("===");
        std::string subString =  result.substr(pos+1);
        std::stringstream foostream(subString);
        std::string line;

        while (std::getline(foostream, line)) {
            if(line.empty())
                continue;
            std::pair<std::string, std::string> devPair = PdmUtils::splitStringInTwo(line);
            m_smartdeviceInfoMap.insert(devPair);
        }
    }
}

/* This is the sample of the dev info output from smartctl
 *
 *  === START OF INFORMATION SECTION ===
 * Vendor:               LG Elect
 * Product:              Ultra Slim Exter
 * Revision:             0103
 * Compliance:           SPC-4
 * User Capacity:        1,000,204,885,504 bytes [1.00 TB]
 * Logical block size:   512 bytes
 * scsiModePageOffset: response length too short, resp_len=12 offset=12 bd_len=8
 * scsiModePageOffset: response length too short, resp_len=12 offset=12 bd_len=8 */

void PdmSmartInfo::pdmSmartLogDevInfo()
{
    std::map<std::string,std::string>::iterator itDev;
    for (itDev=m_smartdeviceInfoMap.begin(); itDev!=m_smartdeviceInfoMap.end(); ++itDev)
    {
        std::string logString = "\"index\":\"" + itDev->first + ",\"info\":\"" + itDev->second + "\"";
        std::list<std::string>::const_iterator listIterator = std::find(importantDevInfo.begin(), importantDevInfo.end(), itDev->first);
        if(listIterator != importantDevInfo.end())
            PDM_LOG_INFO("PdmSmartInfo:",0,"PDM_SMART: %s", logString.c_str());
        else
            PDM_LOG_DEBUG("PDM_SMART: %s", logString.c_str());
    }
}

void PdmSmartInfo::pdmSmartGetAttributes()
{
    std::string sysCommand = "smartctl -A -d sat -T permissive " +  m_deviceName;
    std::string result = PdmUtils::execShellCmd(sysCommand);

    if(!result.empty()) {
        int attrPos = result.find("RAW_VALUE"); // Find the keyword
        std::string attrSubString =  result.substr(attrPos+10);
        std::stringstream attrStream(attrSubString); // This contains all the info to be parsed
        std::string attrEachLine;
        // Create a vector for every attribute with its info. Each line has info one one attribute
        std::vector<std::vector<std::string>>::iterator itAttrInfo = m_attributeInfo.begin();
        while (std::getline(attrStream, attrEachLine)) {
            if(!attrEachLine.empty()) {
                std::stringstream singleAttrStream(attrEachLine);
                std::istream_iterator<std::string> begin(singleAttrStream);
                std::istream_iterator<std::string> end;
                std::vector<std::string> vstrings(begin, end);
                itAttrInfo = m_attributeInfo.insert(itAttrInfo,vstrings);
            }
        }
    }
}

/* This is the sample of the attributes info output from smartctl
 *
 * === START OF READ SMART DATA SECTION ===
 * SMART Attributes Data Structure revision number: 16
 * Vendor Specific SMART Attributes with Thresholds:
 * ID# ATTRIBUTE_NAME          FLAG     VALUE WORST THRESH TYPE      UPDATED  WHEN_FAILED RAW_VALUE
 *   1 Raw_Read_Error_Rate     0x002f   100   100   051    Pre-fail  Always       -       0
 *   2 Throughput_Performance  0x0026   252   252   000    Old_age   Always       -       0
 *   3 Spin_Up_Time            0x0023   087   087   025    Pre-fail  Always       -       4034
 *   4 Start_Stop_Count        0x0032   100   100   000    Old_age   Always       -       598
 *   5 Reallocated_Sector_Ct   0x0033   252   252   010    Pre-fail  Always       -       0
 *   7 Seek_Error_Rate         0x002e   252   252   051    Old_age   Always       -       0
 *   8 Seek_Time_Performance   0x0024   252   252   015    Old_age   Offline      -       0
 *   9 Power_On_Hours          0x0032   100   100   000    Old_age   Always       -       70
 *  10 Spin_Retry_Count        0x0032   252   252   051    Old_age   Always       -       0
 *  12 Power_Cycle_Count       0x0032   099   099   000    Old_age   Always       -       1231
 * 191 G-Sense_Error_Rate      0x0022   252   252   000    Old_age   Always       -       0
 * 192 Power-Off_Retract_Count 0x0022   252   252   000    Old_age   Always       -       0
 * 194 Temperature_Celsius     0x0002   064   060   000    Old_age   Always       -       27 (Min/Max 21/41)
 * 195 Hardware_ECC_Recovered  0x003a   100   100   000    Old_age   Always       -       0
 * 196 Reallocated_Event_Count 0x0032   252   252   000    Old_age   Always       -       0
 * 197 Current_Pending_Sector  0x0032   252   252   000    Old_age   Always       -       0
 * 198 Offline_Uncorrectable   0x0030   252   252   000    Old_age   Offline      -       0
 * 199 UDMA_CRC_Error_Count    0x0036   200   200   000    Old_age   Always       -       0
 * 200 Multi_Zone_Error_Rate   0x002a   100   100   000    Old_age   Always       -       0
 * 223 Load_Retry_Count        0x0032   100   100   000    Old_age   Always       -       1
 * 225 Load_Cycle_Count        0x0032   100   100   000    Old_age   Always       -       1863 */

void PdmSmartInfo::pdmSmartLogAttributeInfo()
{
    std::vector<std::vector<std::string>>::iterator itAttrInfo;

    for (itAttrInfo=m_attributeInfo.begin(); itAttrInfo!=m_attributeInfo.end(); ++itAttrInfo){
        std::vector<std::string> vectStrings = *itAttrInfo;
        int index = 0;
        std::string logString;
        // This condition occurs when last attribute info has some extra spaces. Eg. RAW_VALUE info for id:194
        if(vectStrings.size() > attributeHeaders.size()) {
            int toBeCopiedIndex = attributeHeaders.size() -1;
            // There should be only 9 items for a attribute. So a vector with more than 9 items means the last value
            // has some extra spaces which need to be taken care of. So just append all the extra strings to the last item
            for(unsigned int vIndex = attributeHeaders.size();vIndex < vectStrings.size(); ++vIndex) {
                vectStrings[toBeCopiedIndex] = vectStrings[toBeCopiedIndex] + vectStrings[vIndex];
            }
            vectStrings.erase ((vectStrings.begin()+(toBeCopiedIndex+1)),(vectStrings.begin()+(vectStrings.size())));
        }
        for (std::vector<std::string>::iterator itEachAttr = vectStrings.begin();itEachAttr!=vectStrings.end(); ++itEachAttr){
            logString.append("\"" + attributeHeaders[index] + "\"" + " " + *itEachAttr + " ");
            ++index;
        }
        std::list<std::string>::const_iterator listIterator = std::find(importantAttributeInfoId.begin(), importantAttributeInfoId.end(), vectStrings[0]);
        if(listIterator != importantAttributeInfoId.end())
            PDM_LOG_INFO("PdmSmartInfo:",0,"PDM_SMART: %s", logString.c_str());
        else
            PDM_LOG_DEBUG("PDM_SMART: %s", logString.c_str());
    }
}


void PdmSmartInfo::logPdmSmartInfo()
{
    PDM_LOG_DEBUG("PDM_SMART:%s line: %d devName: %s", __FUNCTION__, __LINE__, m_deviceName.c_str());
    if(!m_deviceName.empty()) {
        // Log the device info
        pdmSmartGetDeviceInfo();
        if(!m_smartdeviceInfoMap.empty())
            pdmSmartLogDevInfo();
        // Log the health info
        pdmSmartGetHealth();
        // Log the attributes info
        pdmSmartGetAttributes();
        if(!m_attributeInfo.empty())
            pdmSmartLogAttributeInfo();
    }
}
