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

#ifndef DISKPARTIONINFO_H_
#define DISKPARTIONINFO_H_

#include <mutex>
#include <string>

#include "Storage.h"

enum FsckState {PARTITION_FSCK_NONE = 0, PARTITION_FSCK_STARTED = 1, PARTITION_FSCK_SUCCESS = 2, PARTITION_FSCK_FAIL = 3, PARTITION_FSCK_TIMEOUT = 4 };

class PdmNetlinkEvent;

class DiskPartitionInfo : public Storage {

private:
    bool m_isMounted;
    int m_fsckStatus;
    bool m_isSupportedFS;
    std::string driveStatus;
    std::mutex cmdExectionLock;

public:
    DiskPartitionInfo(PdmConfig* const pConfObj, PluginAdapter* const pluginAdapter);
    ~DiskPartitionInfo() = default;
    bool isMounted(){ return m_isMounted;}
    void setPartitionInfo(PdmNetlinkEvent* pNE,const std::string &deviceRootPath);
    bool isSupportedFs() { return m_isSupportedFS;}
    void setIsSupportedFs(bool isSupported) { m_isSupportedFS = isSupported;}
    const std::string getDriveStatus(){return driveStatus;}
    void setFsType(const std::string &type);
    void setDriveStatus(std::string dStatus);
    void setVolumeLabel(const std::string &label);
    void setFsckStatus(int fsckStatus) { m_fsckStatus = fsckStatus; }
    int getFsckStatus() { return m_fsckStatus; }
    bool partitionLock();
    void partitionUnLock();
    bool isPartitionMounted(std::string hubPortPath);
};
#endif //DISKPARTIONINFO_H_
