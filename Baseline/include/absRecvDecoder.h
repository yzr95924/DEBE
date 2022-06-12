/**
 * @file absRecvDecoder.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of abstract recv decoder
 * @version 0.1
 * @date 2021-02-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ABS_RECV_DECODER_H
#define ABS_RECV_DECODER_H

#include "configure.h"
#include "sslConnection.h"
#include "absDatabase.h"
#include "cryptoPrimitive.h"
#include "readCache.h"
#include "clientVar.h"

extern Configure config;

class AbsRecvDecoder {
    protected:
        string myName_ = "AbsRecvDecoder";
        // to handle the ssl connection
        SSLConnection* serverChannel_;

        // to query the path of the file recipe 
        string recipeNamePrefix_;
        string recipeNameTail_;

        // restore container cache 
        string containerNamePrefix_;
        string containerNameTail_;

        // read recipe batch size
        uint64_t sendChunkBatchSize_;
        uint64_t sendRecipeBatchSize_;

        // 
        uint64_t totalRestoreRecipeNum_ = 0;
        uint64_t readFromCacheNum_ = 0;
        uint64_t readFromContainerFileNum_ = 0;
    public:
        /**
         * @brief Construct a new Abs Recv Decoder object
         * 
         * @param serverChannel ssl connection pointer
         */
        AbsRecvDecoder(SSLConnection* serverChannel);

        /**
         * @brief Destroy the Abs Recv Decoder object
         * 
         */
        virtual ~AbsRecvDecoder();

        /**
         * @brief the main process
         * 
         * @param outClient the out-enclave client ptr
         */
        virtual void Run(ClientVar* outClient) = 0;
};

#endif