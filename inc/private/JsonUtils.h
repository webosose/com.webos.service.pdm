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

#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <luna-service2/lunaservice.h>
#include <pbnjson.hpp>

#define SCHEMA_V2_PROP(name, type, ...)                  "\"" #name "\":{\"type\":\"" #type "\"" __VA_ARGS__ "}"
#define SCHEMA_V2_OBJECT(name)                           SCHEMA_V2_PROP(name, object)
#define SCHEMA_V2_SYSTEM_PARAMETERS                      SCHEMA_V2_OBJECT($activity)
#define SCHEMA_V2_1(required,p1)                         "{\"type\":\"object\",\"additionalProperties\":false" required ",\"properties\":{" SCHEMA_V2_SYSTEM_PARAMETERS "," p1 "}}"
#define SCHEMA_V2_2(required,p1,p2)                      "{\"type\":\"object\",\"additionalProperties\":false" required ",\"properties\":{" SCHEMA_V2_SYSTEM_PARAMETERS "," p1 "," p2 "}}"
#define SCHEMA_V2_3(required,p1,p2,p3)                   "{\"type\":\"object\",\"additionalProperties\":false" required ",\"properties\":{" SCHEMA_V2_SYSTEM_PARAMETERS "," p1 "," p2 "," p3 "}}"

#define LSERROR_CHECK_AND_PRINT(ret, lsError)\
     do {                          \
         if (ret == false) {               \
             LSErrorPrintAndFree(&lsError);\
             return false; \
         }                 \
     } while (0)

#define VALIDATE_SCHEMA_AND_RETURN(lsHandle, message, schema) {\
                                                                LSMessageJsonParser jsonParser(schema); \
                                                                if (!jsonParser.parse(message,schema, lsHandle))\
                                                                return true;\
                                                               }

class LSMessageJsonParser {
 public:
    LSMessageJsonParser(const std::string &schema);
    bool parse(LSMessage * message, const std::string &schema,LSHandle *sh);
    void LSErrorPrintAndFree(LSError *ptrLSError);

 private:
    std::string mSchemaText;
    pbnjson::JSchemaFragment mSchema;
    pbnjson::JDomParser mParser;
};

#endif //JSONUTILS_H
