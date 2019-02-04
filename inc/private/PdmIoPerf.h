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

#ifndef PDM_IOPERF_H_
#define PDM_IOPERF_H_

#include "PdmErrors.h"
#include <string>

enum TestMode { SEQ_WRITE = 1,SEQ_READ, RAN_READ};
const std::string IO_TESTFILE_NAME = "pdm_io.test";

class PdmIoPerf {

private:
    std::string    m_drvName;
    std::string    m_testFilePath;
    int    m_fd;
    unsigned int m_chunkSize;
    unsigned long long m_fileSize;
    double     m_seqAvgWrite;
    double     m_seqMinWrite;
    double     m_seqAvgRead;
    double     m_seqMinRead;
    double     m_ranAvgRead;
    double     m_ranMinRead;
private:
    PdmDevStatus readWriteIOTest(TestMode mode);
    bool isAllignedSize(unsigned int chunkSize);
    PdmDevStatus seqWriteTest();
    PdmDevStatus seqReadTest();
    PdmDevStatus ranReadTest();
    PdmDevStatus getRanReadOffset(unsigned long long *pRandomReadOffset, unsigned int numRandom);
    void PDM_Sleep(unsigned int milisec);
public:
    PdmIoPerf();
    ~PdmIoPerf()= default;
    PdmDevStatus performIOTest(const std::string& driveName,const std::string& mountName,unsigned int chunkSize);
    double getSeqAvgWrite() { return m_seqAvgWrite; }
    double getSeqMinWrite() { return m_seqMinWrite; }
    double getSeqAvgRead() { return m_seqAvgRead; }
    double getSeqMinRead() { return m_seqMinRead; }
    double getRanAvgRead() { return m_ranAvgRead; }
    double getRanMinRead() { return m_ranMinRead; }
    unsigned int getChunkSize() { return m_chunkSize; }
    std::string getDrvName() { return m_drvName; }
    std::string getTestPath() { return m_testFilePath; }

};
#endif //PDM_IOPERF_H_
