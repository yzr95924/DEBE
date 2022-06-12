/**
 * @file dataSender.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of data sender.
 * @version 0.1
 * @date 2021-01-22
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef DATA_SENDER_H
#define DATA_SENDER_H

#include "configure.h"
#include "sslConnection.h"
#include "messageQueue.h"
#include "cryptoPrimitive.h"

extern Configure config;

class DataSender {
    private:
        string myName_ = "DataSender";
        SSLConnection* dataSecureChannel_;
        pair<int, SSL*> conChannelRecord_;
        
        // config
        uint64_t sendChunkBatchSize_ = 0;
        uint32_t clientID_;

        // for security channel encryption
        CryptoPrimitive* cryptoObj_;
        uint8_t sessionKey_[CHUNK_HASH_SIZE];
        EVP_CIPHER_CTX* cipherCtx_;
        EVP_MD_CTX* mdCtx_;

        uint64_t batchNum_ = 0;
        
        // the sender buffer 
        SendMsgBuffer_t sendChunkBuf_;
        SendMsgBuffer_t sendEncBuffer_;
        MessageQueue<Data_t>* inputMQ_;

        double totalTime_ = 0;

        /**
         * @brief insert a chunk to the sending buffer
         * 
         * @param inputChunk the reference to the input chunk
         */
        void InsertChunkToSenderBuffer(Chunk_t& inputChunk);

        /**
         * @brief send generated the segment package of current segment
         * 
         * @return true success
         * @return false fail
         */
        void SendCurrentSegment();

        /**
         * @brief send the recipe end
         * 
         * @param recipeHead the recipe end
         */
        void SendRecipeEnd(Data_t& recipeHead);

        /**
         * @brief process the recipe end
         * 
         * @param recipeHead the pointer to the recipe end
         */
        void ProcessRecipeEnd(FileRecipeHead_t& recipeHead);

        /**
         * @brief process a chunk
         * 
         * @param inputChunk the input chunk
         */
        void ProcessChunk(Chunk_t& inputChunk);

        /**
         * @brief send a batch of chunks
         * 
         * @param chunkBuffer the chunk buffer
         */
        void SendChunks();
    public:
        /**
         * @brief Construct a new DataSender object
         * 
         * @param dataSecureChannel the pointer to the dataSec
         */
        DataSender(SSLConnection* dataSecureChannel);

        /**
         * @brief Destroy the DataSender object
         * 
         */
        ~DataSender();

        /**
         * @brief the main process of DataSender
         * 
         */
        void Run();

        /**
         * @brief send the upload login with the master key
         * 
         * @param localSecret the client local secret
         * @param fileNameHash the hash of the file name
         */
        void UploadLogin(string localSecret, uint8_t* fileNameHash);
        
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
         */
        void SetSessionKey(uint8_t* sessionKey, size_t keySize) {
            memcpy(sessionKey_, sessionKey, keySize);
            return ;
        }

        /**
         * @brief Set the InputMQ object
         * 
         * @param inputMQ the input MQ
         */
        void SetInputMQ(MessageQueue<Data_t>* inputMQ) {
            inputMQ_ = inputMQ;
            return ;
        }
};

#endif