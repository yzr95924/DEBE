/**
 * @file recvDecoder.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of the recvdecoder
 * @version 0.1
 * @date 2021-09-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef RECV_DECODER_H
#define RECV_DECODER_H

#include <lz4.h>
#include "configure.h"
#include "cryptoPrimitive.h"
#include "readCache.h"
#include "absDatabase.h"
#include "sslConnection.h"
#include "clientVar.h"
#include "absRecvDecoder.h"

extern Configure config;

class RecvDecoder : public AbsRecvDecoder {
    private:
        string myName_ = "RecvDecoder";

        /**
         * @brief recover a chunk
         * 
         * @param chunkBuffer the chunk buffer
         * @param chunkSize the chunk size
         * @param restoreChunkBuf the restore chunk buffer
         */
        void RecoverOneChunk(uint8_t* chunkBuffer, uint32_t chunkSize,
            SendMsgBuffer_t* restoreChunkBuf);

        /**
         * @brief process a batch of recipe
         * 
         * @param recipeBuffer the read recipe buffer
         * @param recipeNum the read recipe num
         * @param curClient the current client var
         */
        void ProcessRecipeBatch(uint8_t* recipeBuffer, size_t recipeNum, 
            ClientVar* curClient);

        /**
         * @brief process the tail batch of the recipe
         * 
         * @param curClient the current client var
         */
        void ProcessRecipeTailBatch(ClientVar* curClient);

        /**
         * @brief Get the Required Containers object 
         * 
         * @param curClient the current client ptr
         */
        void GetReqContainers(ClientVar* curClient);

        /**
         * @brief send the restore chunk to the client
         * 
         * @param sendChunkBuf the send chunk buffer
         * @param clientSSL the ssl connection
         */
        void SendBatchChunks(SendMsgBuffer_t* sendChunkBuf, 
            SSL* clientSSL);

    public:
        /**
         * @brief Construct a new Recv Decoder object
         * 
         * @param serverChannel the ssl connection pointer
         */
        RecvDecoder(SSLConnection* serverChannel);

        /**
         * @brief Destroy the Recv Decoder object
         * 
         */
        ~RecvDecoder();

        /**
         * @brief the main process
         * 
         * @param curClient the current client ptr
         */
        void Run(ClientVar* curClient);
};

#endif