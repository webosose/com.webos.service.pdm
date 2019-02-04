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

#include "JsonUtils.h"
#include "PdmLogUtils.h"
#include <string>

LSMessageJsonParser::LSMessageJsonParser(const std::string &schema ): mSchema(schema)
{
}

bool LSMessageJsonParser::parse(LSMessage * message,const std::string &schema, LSHandle *sh)
{
    const char *payload = LSMessageGetPayload(message);
    // Parse the message with given schema.
    if ((payload) && (!mParser.parse(payload,mSchema)))
    {
        bool bRetVal;
        LSError error;
        LSErrorInit(&error);
        pbnjson::JValue cmdRply = pbnjson::Object();
        cmdRply.put("returnValue", false);
        cmdRply.put("errorCode", 6);
        cmdRply.put("errorText", "Json message parse error");
        bRetVal  =  LSMessageReply (sh,  message,  cmdRply.stringify(NULL).c_str() ,  &error);
        LSERROR_CHECK_AND_PRINT(bRetVal, error);
        return false;
    }
    // Message successfully parsed with given schema
    return true;
}

void LSMessageJsonParser::LSErrorPrintAndFree(LSError *ptrLSError)
{
    if(ptrLSError != NULL)
    {
        LSErrorPrint(ptrLSError, stderr);
        LSErrorFree(ptrLSError);
    }
}