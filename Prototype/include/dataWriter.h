/**
 * @file dataWriter.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of data writer
 * @version 0.1
 * @date 2020-02-01
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef BASICDEDUP_DATA_WRITER_H
#define BASICDEDUP_DATA_WRITER_H

#include "messageQueue.h"
#include "configure.h"
#include "chunkStructure.h"

#include <string>
#include <bits/stdc++.h>

using namespace std;

class DataWriter {
    private:
        string myName_ = "DataWriter";
        // the prefix of container path
        string containerNamePrefix_;

        // the tail of the container path
        string containerNameTail_;

        // the num of the written containers 
        uint64_t containerNum_ = 0;

#if (DATAWRITER_BREAKDOWN == 1)
        // the time of writing container
        double writeTime_ = 0;
#endif
        double totalTime_ = 0;
        // ThreadPool* threadPool_;

    public:

        /**
         * @brief Construct a new Data Writer object
         * 
         */
        DataWriter();

        /**
         * @brief Destroy the Data Writer object
         * 
         */
        ~DataWriter();

        /**
         * @brief the main process of data writer
         * 
         * @param inputMQ the input MQ
         */
        void Run(MessageQueue<Container_t>* inputMQ);

        /**
         * @brief write the container to the storage backend 
         * 
         * @param newContainer the input container 
         */
        void SaveToFile(Container_t& newContainer);
};

#endif // !BASICDEDUP_DATA_WRITER_H




