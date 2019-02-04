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

#include "PdmIoPerf.h"
#include "PdmLogUtils.h"
extern "C" {
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
}
#include <cstring>

#define    PDM_T_REPEAT_WRITE    50
#define    PDM_T_REPEAT_READ     50
#define    PDM_T_WRITE_SIZE      (8 * PDM_MEGA)
#define    PDM_T_REPEAT_SIZE     (16 * PDM_MEGA)
#define    PDM_T_MIN_CHUNKSIZE      4
#define    PDM_T_WRITESIZE       (PDM_T_MIN_CHUNKSIZE * PDM_KILO)
#define    PDM_T_FILESIZE        (10 * PDM_T_WRITESIZE)
#define    PDM_T_READSIZE        (1024 * PDM_KILO)
#define    PDM_T_ALIGNSIZE       (4 * PDM_KILO)
#define    PDM_KILO              (1024)
#define    PDM_MEGA              (PDM_KILO*1024)
#define    PDM_GIGA              (PDM_MEGA*1024)
#define    PDM_TERA              (PDM_GIGA*1024)

PdmIoPerf::PdmIoPerf()
    : m_drvName("")
    , m_testFilePath("")
    , m_fd(0)
    , m_chunkSize(0)
    , m_fileSize(0)
    , m_seqAvgWrite(0)
    , m_seqMinWrite(0)
    , m_seqAvgRead(0)
    , m_seqMinRead(0)
    , m_ranAvgRead(0)
    , m_ranMinRead(0)
{
}

PdmDevStatus PdmIoPerf::performIOTest(const std::string& driveName,const std::string& mountName,unsigned int chunkSize)
{
    PDM_LOG_DEBUG("PdmIoPerf:%s line: %d", __FUNCTION__, __LINE__);
    PdmDevStatus result = PdmDevStatus::PDM_DEV_ERROR;
    if(!driveName.empty())
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d IO Perf test on drive: %s\n", __FUNCTION__, __LINE__, driveName.c_str());
    else if(!mountName.empty())
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d IO Perf test on target path: %s\n", __FUNCTION__, __LINE__, mountName.c_str());

    m_testFilePath = mountName + "/" + IO_TESTFILE_NAME;
    m_chunkSize = chunkSize;
    m_drvName = driveName;

    result = readWriteIOTest(SEQ_WRITE);
    if(result != PdmDevStatus::PDM_DEV_SUCCESS)
        return result;

    result = readWriteIOTest(SEQ_READ);
    if(result != PdmDevStatus::PDM_DEV_SUCCESS)
        return result;

    result = readWriteIOTest(RAN_READ);

    if (result != PdmDevStatus::PDM_DEV_SUCCESS)
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Failed to perform IO Performance task", __FUNCTION__, __LINE__);
    else
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Successful to perform IO Performance task", __FUNCTION__, __LINE__);
    return result;
}

bool PdmIoPerf::isAllignedSize(unsigned int chunkSize)
{
    if (chunkSize == 1)
        return true;
    else if (chunkSize%2 != 0)
        return false;
    else
        return isAllignedSize(chunkSize/2);
}

PdmDevStatus PdmIoPerf::readWriteIOTest(TestMode mode)
{
    PDM_LOG_DEBUG("PdmIoPerf:%s line: %d", __FUNCTION__, __LINE__);
    PdmDevStatus result = PdmDevStatus::PDM_DEV_ERROR;
    if (m_chunkSize < 4 || m_chunkSize > 4096)
        return result;
    if (!isAllignedSize(m_chunkSize))
    {
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d chunk size need to 512 byte boundaries\n", __FUNCTION__, __LINE__);
        return result;
    }
    if(mode == TestMode::SEQ_WRITE)
    {
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d file open for weq write test : %s\n", __FUNCTION__, __LINE__, m_testFilePath.c_str());
        m_fd = open(m_testFilePath.c_str(), O_CREAT | O_WRONLY | O_TRUNC | O_LARGEFILE | O_DIRECT, S_IRWXU | S_IRWXG | S_IRWXO);
        if(m_fd < 0)
        {
            PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Storage write test Open Error! path : %s\n", __FUNCTION__, __LINE__, m_testFilePath.c_str());
            return result;
        }

        result = seqWriteTest();
    }
    else
    {
        m_fd = open(m_testFilePath.c_str(), O_RDONLY, S_IRWXU);
        if(m_fd < 0) {
            PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Storage read test Open Error! path : %s\n", __FUNCTION__, __LINE__, m_testFilePath.c_str());
            return result;
        }

        lseek(m_fd, 0, SEEK_SET);
        m_fileSize = lseek(m_fd, 0, SEEK_END);

        if (m_fileSize == 0) {
            PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Storage read test(%d) - Zero file size => Skip Test\n", __FUNCTION__, __LINE__, mode);
            close(m_fd);
            return result;
        }

        if (mode == TestMode::SEQ_READ)
            result = seqReadTest();
        else
            result = ranReadTest();
    }
    close(m_fd);
    return result;
}

PdmDevStatus PdmIoPerf::seqWriteTest()
{
    PDM_LOG_DEBUG("PdmIoPerf:%s line: %d", __FUNCTION__, __LINE__);
    PdmDevStatus result = PdmDevStatus::PDM_DEV_ERROR;
    int writeResult;
    unsigned int chunkSizeKB, cntRepeat = 0;
    unsigned long long writeSize, totalTimeDiff = 0, timeDiff;
    char *pCurBuf = NULL;
    double tWriteSpeed;
    struct timespec start_time, end_time;
    unsigned long long totalWrite = 0;

    chunkSizeKB = m_chunkSize;
    m_seqMinWrite = 1000000;
    writeSize = (unsigned long long)chunkSizeKB * PDM_KILO;

    pCurBuf = (char *)memalign(PDM_T_ALIGNSIZE, writeSize);
    if(pCurBuf == NULL)
    {
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Storage SeqWriteTask- Memory Alloc Fail!\n", __FUNCTION__, __LINE__);
        close(m_fd);
        return result;
    }
    fsync(m_fd);
    memset(pCurBuf, 'j', writeSize);

    PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Storage Sequential Write Test start - chunkSize[%d KB] filePath[%s]\n", __FUNCTION__, __LINE__, chunkSizeKB, m_testFilePath.c_str());

    while(totalWrite < PDM_T_WRITE_SIZE)
    {
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Storage SeqWriteTask(%d)-Write Test\n", __FUNCTION__, __LINE__, ++cntRepeat);

        clock_gettime(CLOCK_REALTIME, &start_time);
        writeResult = write(m_fd, pCurBuf, writeSize);
        clock_gettime(CLOCK_REALTIME, &end_time);

        if (writeResult < 0)
        {
            PDM_LOG_DEBUG("PdmIoPerf:%s line: %d StorageRWTest_SeqWrite-Write fail1(%d) => Skip Test(%d)\n", __FUNCTION__, __LINE__, cntRepeat, writeResult);
            free(pCurBuf);
            close(m_fd);
            return result;
        }

        timeDiff = (((unsigned long long)(end_time.tv_sec - start_time.tv_sec))*1000000) + (unsigned long long)((end_time.tv_nsec - start_time.tv_nsec)/1000);
        tWriteSpeed = 0;
        if (timeDiff)
        {
            tWriteSpeed = (((double)writeResult / timeDiff ) * 1000000)/PDM_MEGA;
            totalTimeDiff += timeDiff;
            if (m_seqMinWrite > tWriteSpeed) m_seqMinWrite = tWriteSpeed;
        }
        totalWrite += writeResult;
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d %d KB write test - timeDiff : %lld, speed : %f MByte/s\n", __FUNCTION__, __LINE__, writeResult, timeDiff, tWriteSpeed);

        PDM_Sleep(5);
    }

    if (totalTimeDiff) {
        m_seqAvgWrite = (((double)totalWrite / totalTimeDiff ) * 1000000)/PDM_MEGA;
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d average write speed : %f MByte/s, min write speed : %f MByte/s, totalWrite : %lld\n", __FUNCTION__, __LINE__, m_seqAvgWrite, m_seqMinWrite, totalWrite);
    }

    free(pCurBuf);
    close(m_fd);
    m_fd = -1;

    return PdmDevStatus::PDM_DEV_SUCCESS;
}
PdmDevStatus PdmIoPerf::seqReadTest()
{
    PDM_LOG_DEBUG("PdmIoPerf:%s line: %d", __FUNCTION__, __LINE__);
    PdmDevStatus result = PdmDevStatus::PDM_DEV_ERROR;
    unsigned int readresult;
    unsigned int readSize, cnt = 0;
    char *pCurBuf = NULL;
    double tReadSpeed;
    unsigned long long totalRead = 0, totalTimeDiff = 0, timeDiff;

    struct timespec start_time, end_time;

    m_seqMinRead = 1000000;
    readSize = m_chunkSize * PDM_KILO;

    pCurBuf = (char *)memalign(PDM_T_ALIGNSIZE, readSize);
    if(pCurBuf == NULL) {
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Storage SeqReadTask- Memory Alloc Fail!\n", __FUNCTION__, __LINE__);
        close(m_fd);
        return result;
    }

    PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Storage Sequential Read Test start - chunkSize[%d KB] filePath[%s] m_fileSize[%lld]\n", __FUNCTION__, __LINE__, m_chunkSize, m_testFilePath.c_str(), m_fileSize);
    fsync(m_fd);
    lseek(m_fd, 0, SEEK_SET);

    do
    {
        PDM_Sleep(5);

        clock_gettime(CLOCK_REALTIME, &start_time);
        readresult = read(m_fd, pCurBuf, readSize);
        clock_gettime(CLOCK_REALTIME, &end_time);

        if (readresult != readSize)    {
            PDM_LOG_DEBUG("PdmIoPerf:%s line: %d StorageRWTest_SeqRead - Read fail => Skip Test (readsult : %d, readSize: %d)\n", __FUNCTION__, __LINE__, readresult, readSize);
            close(m_fd);
            free(pCurBuf);
            return result;
        }

        timeDiff = (((unsigned long long)(end_time.tv_sec - start_time.tv_sec))*1000000) + (unsigned long long)((end_time.tv_nsec - start_time.tv_nsec)/1000);
        tReadSpeed = 0;
        if (timeDiff) {
            tReadSpeed = (((double)readresult / timeDiff ) * 1000000)/PDM_MEGA;
            totalTimeDiff += timeDiff;

            if (m_seqMinRead > tReadSpeed) m_seqMinRead = tReadSpeed;
        }
        totalRead += readresult;
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d [%d} %d KB read test - timeDiff : %lld, speed : %f MByte/s\n", __FUNCTION__, __LINE__, ++cnt, m_chunkSize, timeDiff, tReadSpeed);

        fsync(m_fd);
    }while (totalRead < m_fileSize && readresult != 0);

    if (totalTimeDiff) {
        m_seqAvgRead = (((double)totalRead / totalTimeDiff ) * 1000000)/PDM_MEGA;
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d average read speed : %f MByte/s, min read speed : %f\n", __FUNCTION__, __LINE__, m_seqAvgRead, m_seqMinRead);
    }

    free(pCurBuf);
    close(m_fd);
    m_fd = -1;

    return PdmDevStatus::PDM_DEV_SUCCESS;
}
PdmDevStatus PdmIoPerf::ranReadTest()
{
    PDM_LOG_DEBUG("PdmIoPerf:%s line: %d", __FUNCTION__, __LINE__);
    PdmDevStatus result = PdmDevStatus::PDM_DEV_ERROR;
    int readresult;
    unsigned int readSize, numRandom, loopi;
    unsigned long long offset, totalRead = 0, totalTimeDiff = 0, timeDiff, *pRandomReadOffset;
    char *pCurBuf = NULL;
    double tReadSpeed;

    struct timespec start_time, end_time;

    if (m_chunkSize == 0){
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Param error!\n", __FUNCTION__, __LINE__);
        return result;
    }

    readSize = m_chunkSize * PDM_KILO;
    m_ranMinRead = 1000000;

    numRandom = (unsigned int)m_fileSize/(m_chunkSize*PDM_KILO);
    pRandomReadOffset = (unsigned long long*)malloc(sizeof(unsigned long long)*numRandom);

    if (!pRandomReadOffset)
    {
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d malloc fail!\n", __FUNCTION__, __LINE__);
        close(m_fd);
        return result;
    }
    if ( getRanReadOffset(pRandomReadOffset, numRandom) ==  PdmDevStatus::PDM_DEV_ERROR)
    {
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d generate ramdom offset Fail!\n", __FUNCTION__, __LINE__);
        free(pRandomReadOffset);
        close(m_fd);
        return result;
    }

    pCurBuf = (char *)memalign(PDM_T_ALIGNSIZE, readSize);
    if(pCurBuf == NULL) {
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Storage RanReadTask- Memory Alloc Fail!\n", __FUNCTION__, __LINE__);
        free(pRandomReadOffset);
        close(m_fd);
        return result;
    }

    PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Storage random Read Test start - chunkSize[%d KB] filePath[%s] m_fd[%d]\n", __FUNCTION__, __LINE__, m_chunkSize, m_testFilePath.c_str(), m_fd);
    fsync(m_fd);

    for (loopi = 0; loopi < numRandom; loopi++)
    {
        system("echo 3 > /proc/sys/vm/drop_caches");
        PDM_Sleep(5);

        offset = pRandomReadOffset[loopi] * readSize;

        lseek(m_fd, offset, SEEK_SET);
        clock_gettime(CLOCK_REALTIME, &start_time);
        readresult = read(m_fd, pCurBuf, readSize);
        clock_gettime(CLOCK_REALTIME, &end_time);

        if (readresult < 0)    {
            PDM_LOG_DEBUG("PdmIoPerf:%s line: %d StorageRWTest_RanRead - Read fail => Skip Test\n", __FUNCTION__, __LINE__);
            close(m_fd);
            free(pCurBuf);
            free(pRandomReadOffset);
            return result;
        }

        timeDiff = (((unsigned long long)(end_time.tv_sec - start_time.tv_sec))*1000000) + (unsigned long long)((end_time.tv_nsec - start_time.tv_nsec)/1000);
        tReadSpeed = 0;
        if (timeDiff)    {
            tReadSpeed = (((double)readresult / timeDiff ) * 1000000)/PDM_MEGA;
            totalTimeDiff += timeDiff;

            if (m_ranMinRead > tReadSpeed) m_ranMinRead = tReadSpeed;
        }
        totalRead += readresult;
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Random read test (offset : %lld, readresult : %d) - timeDiff : %lld, speed : %f MByte/s\n", __FUNCTION__, __LINE__, offset, readresult, timeDiff, tReadSpeed);

        fsync(m_fd);
    }

    if (totalTimeDiff) {
        m_ranAvgRead = (((double)totalRead / totalTimeDiff ) * 1000000)/PDM_MEGA;
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d average read speed : %f MByte/s, min read speed : %f\n", __FUNCTION__, __LINE__, m_ranAvgRead, m_ranMinRead);
    }

    free(pCurBuf);
    free(pRandomReadOffset);
    close(m_fd);

    return PdmDevStatus::PDM_DEV_SUCCESS;
}

PdmDevStatus PdmIoPerf::getRanReadOffset(unsigned long long *pRandomReadOffset, unsigned int numRandom)
{
    PDM_LOG_DEBUG("PdmIoPerf:%s line: %d", __FUNCTION__, __LINE__);
    PdmDevStatus result = PdmDevStatus::PDM_DEV_ERROR;
    unsigned long long tRand, tmp;
    unsigned int i;

    if (pRandomReadOffset){
        for(i = 0; i < numRandom; i++){
            pRandomReadOffset[i] = i;
        }
        for(i = 0; i < numRandom; i++) {
            tRand = lrand48();
            tRand = tRand%numRandom;
            tmp = pRandomReadOffset[i];
            pRandomReadOffset[i] = pRandomReadOffset[tRand];
            pRandomReadOffset[tRand] = tmp;
        }

        result = PdmDevStatus::PDM_DEV_SUCCESS;
    }
    else
    {
        PDM_LOG_DEBUG("PdmIoPerf:%s line: %d Random uniqueness fallback.\n", __FUNCTION__, __LINE__);
    }

    return result;
}

void PdmIoPerf::PDM_Sleep(unsigned int milisec)
{
    PDM_LOG_DEBUG("PdmIoPerf:%s line: %d", __FUNCTION__, __LINE__);
    int microsecs;
    struct timeval tv;

    microsecs = milisec * 1000;
    tv.tv_sec  = microsecs / 1000000;
    tv.tv_usec = microsecs % 1000000;
    select (0, NULL, NULL, NULL, &tv);

    return;
}
