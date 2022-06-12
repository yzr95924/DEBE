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
#include "../build/src/Enclave/storeEnclave_u.h"

class DataReceiver {
    private:
        string myName_ = "DataReceiver";
        // for ssl connection
        SSLConnection* dataSecureChannel_;

        sgx_enclave_id_t eidSGX_;

        uint64_t batchNum_ = 0;
        uint64_t recipeEndNum_ = 0;

        // to pass the data to the index thread
        AbsIndex* absIndexObj_;

        // pass the storage core obj
        StorageCore* storageCoreObj_;

    public:
        /**
         * @brief Construct a new DataReceiver object
         * 
         * @param absIndexObj the pointer to the index obj
         * @param dataSecurity the pointer to the security channel
         * @param eidSGX the sgx id
         */
        DataReceiver(AbsIndex* absIndexObj, SSLConnection* dataSecureChannel, sgx_enclave_id_t eidSGX);

        /**
         * @brief Destroy the DataReceiver object
         * 
         */
        ~DataReceiver();

        /**
         * @brief the main process to handle new client upload-request connection
         * 
         * @param outClient the out-enclave client ptr
         * @param enclaveInfo the pointer to the enclave info 
         */
        void Run(ClientVar* outClient, EnclaveInfo_t* enclaveInfo);
        
        /**
         * @brief Set the Storage Core Obj object
         * 
         * @param storageCoreObj the pointer to the storage core obj
         */
        void SetStorageCoreObj(StorageCore* storageCoreObj) {
            storageCoreObj_ = storageCoreObj;
            return ;
        }
};

#endif