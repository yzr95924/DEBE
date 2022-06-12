/**
 * @file ecallSimpleIndex.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of the simple index inside the enclave
 * @version 0.1
 * @date 2020-12-28
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef ECALL_SIMPLE_INDEX_H
#define ECALL_SIMPLE_INDEX_H

#include "enclaveBase.h"

class EcallOutEnclaveIndex : public EnclaveBase {
    private:
        string myName_ = "EcallOutEnclaveIndex"; 
    public:
        /**
         * @brief Construct a new Ecall Simple Index object
         * 
         */
        EcallOutEnclaveIndex();

        /**
         * @brief Destroy the Ecall Simple Index object
         * 
         */
        ~EcallOutEnclaveIndex();

        /**
         * @brief process one batch
         * 
         * @param recvChunkBuf the recv chunk buffer
         * @param upOutSGX the pointer to enclave-related var 
         */
        void ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, 
            UpOutSGX_t* upOutSGX);

        /**
         * @brief process the tailed batch when received the end of the recipe flag
         * 
         * @param upOutSGX the pointer to enclave-related var
         */
        void ProcessTailBatch(UpOutSGX_t* upOutSGX);
};


#endif