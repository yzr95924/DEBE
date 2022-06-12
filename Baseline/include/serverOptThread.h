/**
 * @file serverOptThread.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the server main thread for each client
 * @version 0.1
 * @date 2021-09-17
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef BASELINE_SERVER_OPT_THREAD_H
#define BASELINE_SERVER_OPT_THREAD_H

// for upload
#include "dataWriter.h"
#include "dataReceiver.h"
#include "absIndex.h"
#include "plainIndex.h"

// for basice build block
#include "factoryDatabase.h"
#include "absDatabase.h"
#include "configure.h"
#include "clientVar.h"

// for restore
#include "recvDecoder.h"

extern Configure config;

class ServerOptThread {
    private:
        string myName_ = "ServerOptThread";
        string logFileName_ = "server-log";

        // handlers passed from outside
        SSLConnection* serverChannel_;
        AbsDatabase* fp2ChunkDB_;

        // for upload
        DataReceiver* dataReceiverObj_;
        AbsIndex* absIndexObj_;
        DataWriter* dataWriterObj_;
        StorageCore* storageCoreObj_;

        // for restore
        RecvDecoder* recvDecoderObj_;

        // index type
        int indexType_;

        // the number of received client requests
        uint64_t totalUploadReqNum_ = 0;
        uint64_t totalRestoreReqNum_ = 0;

        // store the client information
        unordered_map<int, boost::mutex*> clientLockIndex_;

        // for log file
        ofstream logFile_;

        std::mutex clientLockSetLock_;

        /**
         * @brief check the file status
         * 
         * @param fullRecipePath the full recipe path
         * @param optType the operation type
         * @return true success
         * @return false fail
         */
        bool CheckFileStatus(string& fullRecipePath, int optType);

    public:
        /**
         * @brief Construct a new Server Opt Thread object
         * 
         * @param serverChannel server communication channel
         * @param fp2ChunkDB the chunk info index
         * @param indexType the index type
         */
        ServerOptThread(SSLConnection* serverChannel, AbsDatabase* fp2ChunkDB, int indexType);

        /**
         * @brief Destroy the Server Opt Thread object
         * 
         */
        ~ServerOptThread();

        /**
         * @brief the main thread 
         * 
         * @param clientSSL the client ssl
         */
        void Run(SSL* clientSSL);
};


#endif