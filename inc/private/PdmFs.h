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

#ifndef PDMFS_H_
#define PDMFS_H_

#include "DiskPartitionInfo.h"
#include "PdmErrors.h"
#include <sys/statfs.h>

typedef struct SpaceInfo {
    uint64_t driveSize;
    uint64_t usedSize;
    uint64_t freeSize;
    uint64_t usedRate;
}SpaceInfo;


class DiskPartitionInfo;

class PdmFs {

private:
    uint64_t mountflags(const std::string &fsType, const bool &readOnly);
    void setDefaultFsForFormat(DiskPartitionInfo *partition, std::string &fileSysType);
public:
    PdmFs() = default;
    ~PdmFs() = default;
    PdmDevStatus format(DiskPartitionInfo *partition, std::string &fileSysType,const std::string &label);
    PdmDevStatus fsck(DiskPartitionInfo &partition, const std::string& fsckMode);
    bool mountPartition(DiskPartitionInfo &partition, const bool &readOnly);
    bool umount(DiskPartitionInfo &partition, const bool lazyUnmount) const;
    PdmDevStatus setVolumeLabel(DiskPartitionInfo *partition, const std::string &volLabel);
    PdmDevStatus isWritable(DiskPartitionInfo *partition, bool &isWritable);
    bool isSupportedFileSystem(const std::string fsType, const std::string &storageType);
    bool isDriveBusy(DiskPartitionInfo &partition) const;
    bool calculateSpaceInfo(const std::string &mountName, SpaceInfo *fsInfo);
    void checkFileSystem(DiskPartitionInfo &partition);
};



#endif /* PDMFS_H_ */
