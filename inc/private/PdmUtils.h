// Copyright (c) 2019-2021 LG Electronics, Inc.
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

#ifndef _PDM_UTILS_H
#define _PDM_UTILS_H

#include <string>
#include <sys/types.h>

namespace PdmUtils
{
    uid_t get_uid(const char *user_name);
    gid_t get_gid(const char *group_name);
    void do_chown(const char *file_path, const char *user_name, const char *group_name);
    std::string execShellCmd(const std::string &cmd);
    bool createDir(const std::string &dirName);
    bool removeDirRecursive(const std::string &dirPath);
    std::string& ltrimString(std::string &str);
    std::string& rtrimString(std::string &str);
    std::string& trimString(std::string &str);
    std::pair<std::string, std::string> splitStringInTwo(std::string stringToSplit);
};

#endif //_PDM_UTILS_H
