/**
 * @file enclaveRecvDecoder.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of enclave-based recvdecoder 
 * @version 0.1
 * @date 2021-02-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ENCLAVE_RECV_DECODER_H
#define ENCLAVE_RECV_DECODER_H

#include "sgx_urts.h"
#include "sgx_capable.h"
#include "../build/src/Enclave/storeEnclave_u.h"
#include "configure.h"
#include "cryptoPrimitive.h"
#include "readCache.h"
#include "absDatabase.h"
#include "sslConnection.h"
#include "absRecvDecoder.h"
#include "clientVar.h"

extern Configure config;

class EnclaveRecvDecoder : public AbsRecvDecoder {
    private:
        string myName_ = "EnclaveRecvDecoder";
        // the variable to record the enclave information 
        sgx_enclave_id_t eidSGX_;
    public:
        /**
         * @brief Construct a new EnclaveRecvDecoder object
         * 
         * @param dataSecureChannel the ssl connection pointer
         * @param eidSGX the id to the enclave
         */
        EnclaveRecvDecoder(SSLConnection* dataSecureChannel, 
            sgx_enclave_id_t eidSGX);

        /**
         * @brief Destroy the Enclave Recv Decoder object
         * 
         */
        ~EnclaveRecvDecoder();

        /**
         * @brief the main process
         * 
         * @param outClient the out-enclave client ptr
         */
        void Run(ClientVar* outClient);

        /**
         * @brief Get the Required Containers object 
         * 
         * @param outClient the out-enclave client ptr
         */
        void GetReqContainers(ClientVar* outClient);

        /**
         * @brief send the restore chunk to the client
         * 
         * @param sendChunkBuf the send chunk buffer
         * @param clientSSL the ssl connection
         */
        void SendBatchChunks(SendMsgBuffer_t* sendChunkBuf, 
            SSL* clientSSL);
};

#endif