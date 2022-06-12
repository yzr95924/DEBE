/**
 * @file absDaE.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of DaE
 * @version 0.1
 * @date 2021-05-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ABE_DAE_H
#define ABE_DAE_H

#include "configure.h"
#include "sslConnection.h"
#include "cryptoPrimitive.h"

extern Configure config;

class AbsDAE {
    protected:        
        string myName_ = "AbsDAE";
        // config
        uint64_t sendChunkBatchSize_ = 0;
        uint32_t clientID_;
        uint8_t masterKey_[CHUNK_HASH_SIZE];

        // key manager channel
        SSLConnection* keyManagerChannel_;
        pair<int, SSL*> keyChannelRecord_;

        // for crypto
        CryptoPrimitive* cryptoObj_;
        EVP_CIPHER_CTX* cipherCtx_;
        EVP_MD_CTX* mdCtx_;

        string keyRecipeLocalSuffix_ = "-client-key";
        ofstream keyRecipeFile_;
        string keyRecipeNameStr_;

    public:
        /**
         * @brief Construct a new AbsDAE object
         * 
         * @param fileNameHash the file name hash
         */
        AbsDAE(uint8_t* fileNameHash);

        /**
         * @brief Destroy the Abs DAE object
         * 
         */
        virtual ~AbsDAE();

        /**
         * @brief process one segment of chunk
         * 
         * @param sendPlainChunkBuf the buffer to the plaintext chunk
         * @param sendCipherChunkBuf the buffer to the ciphertext chunk
         * @param sendRecipeBuf the buffer for the recipe <only store the key>
         */
        virtual void ProcessBatchChunk(SendMsgBuffer_t* sendPlainChunkBuf,
            SendMsgBuffer_t* sendCipherChunkBuf, SendMsgBuffer_t* sendRecipeBuf) = 0;
        
        /**
         * @brief close the connection with key manager
         * 
         */
        virtual void CloseKMConnection() = 0;

        /**
         * @brief Set the Master Key object
         * 
         * @param masterKey the master key
         */
        void SetMasterKey(uint8_t* masterKey) {
            memcpy(masterKey_, masterKey, CHUNK_HASH_SIZE);
            return ;
        }
};


#endif