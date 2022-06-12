/**
 * @file tedDaE.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of ted
 * @version 0.1
 * @date 2021-12-12
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef TED_DAE_H
#define TED_DAE_H

#include "absDaE.h"
#include "murmurHash.h"

class TEDDAE : public AbsDAE {
    private:
        string myName_ = "TED-DAE";

        SendMsgBuffer_t sendShortHashBuf_;

    public:
        /**
         * @brief Construct a new TEDDAE object
         * 
         */
        TEDDAE(uint8_t* fileNameHash);

        /**
         * @brief Destroy the TEDDAE object
         * 
         */
        ~TEDDAE();

        /**
         * @brief process one segment of chunk
         * 
         * @param sendPlainChunkBuf the buffer to the plaintext chunk
         * @param sendCipherChunkBuf the buffer to the ciphertext chunk
         * @param sendRecipeBuf the buffer for the recipe <only store the key>
         */
        void ProcessBatchChunk(SendMsgBuffer_t* sendPlainChunkBuf,
            SendMsgBuffer_t* sendCipherChunkBuf, SendMsgBuffer_t* sendRecipeBuf);
        
        /**
         * @brief close the connection with key manager
         * 
         */
        void CloseKMConnection();
};

#endif