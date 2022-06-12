/**
 * @file restoreWriter.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in restoreWriter.h
 * @version 0.1
 * @date 2020-01-03
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/restoreWriter.h"

/**
 * @brief Construct a new Restore Writer object
 * 
 * @param fileName restore file name
 */
RestoreWriter::RestoreWriter(string fileName) {
    string newFileName;
    newFileName = fileName + "-d";
    outputFile_ = fopen(newFileName.c_str(), "wb");
    if (!outputFile_) {
        tool::Logging(myName_.c_str(), "cannot init the restore file.\n");
        exit(EXIT_FAILURE);
    }
    tool::Logging(myName_.c_str(), "init the RestoreWriter.\n");
}

/**
 * @brief Destroy the Restore Writer object
 * 
 */
RestoreWriter::~RestoreWriter() {
    fprintf(stderr, "========RestoreWriter Info========\n");
    fprintf(stderr, "total thread running time: %lf\n", totalTime_);
#if (RESTORE_WRITER_BREAKDOWN == 1)
    fprintf(stderr, "Restore writer time: %lf\n", restoreWriteTime_);
#endif 
    fprintf(stderr, "write chunk num: %lu\n", totalRecvNum_);
    fprintf(stderr, "write data size: %lu\n", totalWrittenSize_);
    fprintf(stderr, "==================================\n");
}

/**
 * @brief the main process
 * 
 */
void RestoreWriter::Run() {
    tool::Logging(myName_.c_str(),"the main thread is running.\n");
    gettimeofday(&sTotalTime_, NULL);
    Chunk_t newData;
    bool jobDoneFlag = false;

    while (true) {
        // the main loop
        if (inputMQ_->done_ && inputMQ_->IsEmpty()) {
            tool::Logging(myName_.c_str(), "no chunk in the message queue, all jobs are done.\n");
            jobDoneFlag = true;
        }

        if (inputMQ_->Pop(newData)) {
#if (RESTORE_WRITER_BREAKDOWN == 1)
            gettimeofday(&sRestoreTime_, NULL);
#endif 
            fwrite((char*)newData.data, newData.chunkSize, 1, outputFile_);
            totalRecvNum_++;
            totalWrittenSize_ += newData.chunkSize;
#if (RESTORE_WRITER_BREAKDOWN == 1)
            gettimeofday(&eRestoreTime_, NULL);
            restoreWriteTime_ += tool::GetTimeDiff(sRestoreTime, eRestoreTime);
#endif
        } 

        if (jobDoneFlag) {
            break;
        }  
    }

    // ensure to write the data to the disk
    fsync(fileno(outputFile_));
    fclose(outputFile_);
    tool::Logging(myName_.c_str(), "thread exit.\n");
    gettimeofday(&eTotalTime_, NULL);
    totalTime_ += tool::GetTimeDiff(sTotalTime_, eTotalTime_);
    return ;
}