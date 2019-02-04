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

#include "Storage.h"
#include "PdmLogUtils.h"

Storage::Storage(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter, std::string deviceType, std::string errorReason, StorageInterfaceTypes storageType)
            : Device(pConfObj, pluginAdapter, deviceType,errorReason)
            , isMounted(false)
            , isReadOnly(false)
            , powerStatus(true)
            , isDirCreated(false)
            , m_driveSize(0)
            , m_usedSize(0)
            , m_freeSize(0)
            , m_usedRate(0)
            , volumeLabel("")
            , uuid("")
            , driveName("")
            , fsType("")
            , mountName("")
            , m_storageType(storageType)

{
    rootPath = readRootPath();
}

std::string Storage::readRootPath()
{
    std::string rootPathString;
    pbnjson::JValue rootPathConfVal = pbnjson::JValue();
    PdmConfigStatus confErrCode = m_pConfObj->getValue("Common","RootPath",rootPathConfVal);
    if(confErrCode != PdmConfigStatus::PDM_CONFIG_ERROR_NONE)
    {
        PdmErrors::logPdmErrorCodeAndText(confErrCode);
        return rootPathString;
    }
    if (rootPathConfVal.isString())
    {
        rootPathString = rootPathConfVal.asString();
        PDM_LOG_DEBUG("Storage:%s line: %d rootPath: %s", __FUNCTION__, __LINE__, rootPathString.c_str());
    }

    return rootPathString;
}
