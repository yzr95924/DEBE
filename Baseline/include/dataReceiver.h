/**
 * @file dataReceiver.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of dataReceiver
 * @version 0.1
 * @date 2021-01-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef DATA_RECEIVER_H
#define DATA_RECEIVER_H

#include "configure.h"
#include "clientVar.h"
#include "sslConnection.h"
#include "absIndex.h"

#define CLIENT_LOG_FILE "client-time.log"

class DataReceiver {
    private:
        string myName_ = "DataReceiver";
        // for ssl connection
        SSLConnection* serverChannel_;

        uint64_t batchNum_ = 0;
        uint64_t recipeEndNum_ = 0;
        uint64_t recipeBatchNum_ = 0;

        // to pass the data to the index thread
        AbsIndex* absIndexObj_;

        // pass the storage cor obj 
        StorageCore* storageCoreObj_;

        /**
         * @brief process a batch of recipes
         * 
         * @param curClient the client var
         */
        void ProcessRecipes(ClientVar* curClient);

    public:
        /**
         * @brief Construct a new DataReceiver object
         * 
         * @param absIndexObj the pointer to the index obj
         * @param serverChannel the pointer to the security channel
         */
        DataReceiver(AbsIndex* absIndexObj, SSLConnection* serverChannel);

        /**
         * @brief Destroy the DataReceiver object
         * 
         */
        ~DataReceiver();

        /**
         * @brief the main process to handle new client upload-request connection
         * 
         * @param curClient the ptr to the current client
         * @param enclaveInfo the ptr to the enclave info
         */
        void Run(ClientVar* curClient, EnclaveInfo_t* enclaveInfo);

        /**
         * @brief Set the Storage Core Obj object
         * 
         * @param storageCoreObj the ptr to the storageCore obj
         */
        void SetStorageCoreObj(StorageCore* storageCoreObj) {
            storageCoreObj_ = storageCoreObj;
            return ;
        }
};

#endif