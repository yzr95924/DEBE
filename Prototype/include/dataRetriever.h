/**
 * @file dataRetriever.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of data retriever 
 * @version 0.1
 * @date 2021-02-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef DATA_RETRIEVER_H
#define DATA_RETRIEVER_H

#include "configure.h"
#include "constVar.h"
#include "messageQueue.h"
#include "sslConnection.h"
#include "cryptoPrimitive.h"
#include "restoreWriter.h"

class DataRetriever {
    private:
        string myName_ = "DataRetriever";
        SSLConnection* dataSecureChannel_;
        pair<int, SSL*> conChannelRecord_;

        // config
        uint64_t recvChunkBatchSize_;
        uint32_t clientID_;

        // for crypto operation
        CryptoPrimitive* cryptoObj_;
        uint8_t sessionKey_[CHUNK_HASH_SIZE];
        EVP_MD_CTX* mdCtx_; 
        EVP_CIPHER_CTX* cipherCtx_;

        // received data buffer
        uint8_t* decBuffer_;
        SendMsgBuffer_t recvChunkBuf_;
        MessageQueue<Chunk_t>* outputMQ_;
        FileRecipeHead_t fileRecipeHead_;

        double totalTime_ = 0;

        /**
         * @brief process a batch of chunks
         * 
         */
        void ProcessChunkBatch();
    
    public:
        // for statistics: the received chunk number  
        uint64_t _totalRecvChunkNum = 0;
        uint64_t _totalRecvDataSize = 0;

        /**
         * @brief Construct a new Data Retriever object
         * 
         * @param dataSecureChannel the pointer to the data secure channel
         */
        DataRetriever(SSLConnection* dataSecureChannel);

        /**
         * @brief Destroy the Data Retriever object
         * 
         */
        ~DataRetriever();

        /**
         * @brief the main process
         * 
         */
        void Run();

        /**
         * @brief init the restore login with the server 
         * 
         * @param localSecret the client local secret
         * @param fileNameHash the hash of the file name
         */
        void RestoreLogin(string localSecret, uint8_t* fileNameHash);

        /**
         * @brief Set the Connection Record object
         * 
         * @param conChannelRecord the connection record
         */
        void SetConnectionRecord(pair<int, SSL*> conChannelRecord) {
            conChannelRecord_ = conChannelRecord;
            return ;
        }

        /**
         * @brief Set the Session Key object
         * 
         * @param sessionKey the session key
         * @param keySize the key size
         */
        void SetSessionKey(uint8_t* sessionKey, size_t keySize) {
            memcpy(sessionKey_, sessionKey, keySize);
            return ;
        }

        /**
         * @brief Set the Output MQ object
         * 
         * @param outputMQ the output MQ
         */
        void SetOutputMQ(MessageQueue<Chunk_t>* outputMQ) {
            outputMQ_ = outputMQ;
            return ;
        }
};

#endif