/**
 * @file enclaveIndex.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of simple enclave index
 * @version 0.1
 * @date 2020-12-08
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef ENCLAVE_SIMPLE_H
#define ENCLAVE_SIMPLE_H

#include "absIndex.h"

#include "sgx_urts.h"
#include "sgx_capable.h"
#include "../src/Enclave/include/storeOCall.h"
#include "../build/src/Enclave/storeEnclave_u.h"

class EnclaveIndex : public AbsIndex {
    private:
        string myName_ = "EnclaveIndex";
        // the variable to record the enclave information
        sgx_enclave_id_t eidSGX_;
    public:
        /**
         * @brief Construct a new Enclave Simple Index object
         * 
         * @param indexStore the reference to the index store
         * @param indexType the type of index
         * @param eidSGX the enclave id
         */
        EnclaveIndex(AbsDatabase* indexStore, int indexType, sgx_enclave_id_t eidSGX);

        /**
         * @brief Destroy the Enclave Simple Index object
         * 
         */
        ~EnclaveIndex();

        /**
         * @brief process one batch 
         * 
         * @param recvChunkBuf the recv chunk buffer
         * @param upOutSGX the structure to store the enclave related variable
         */
        void ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, UpOutSGX_t* upOutSGX);

        /**
         * @brief process the tail segment
         * 
         * @param upOutSGX the structure to store the enclave related variable
         */
        void ProcessTailBatch(UpOutSGX_t* upOutSGX);
};

#endif