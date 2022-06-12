/**
 * @file plainDaE.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of plain 
 * @version 0.1
 * @date 2021-05-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef PLAIN_DAE_H
#define PLAIN_DAE_H

#include "absDaE.h"

class PlainDAE : public AbsDAE {
    private:
        string myName_ = "Plain-DAE";
    public: 
        /**
         * @brief Construct a new Plain D A E object
         * 
         */
        PlainDAE(uint8_t* fileNameHash);

        /**
         * @brief Destroy the Plain D A E object
         * 
         */
        ~PlainDAE();

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