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
#include "absDaE.h"

extern Configure config;

class DataSender {
    private:
        string myName_ = "DataSender";
        uint32_t clientID_;

        // for communication
        SSLConnection* serverChannel_;
        pair<int, SSL*> conChannelRecord_;

        // config
        uint64_t sendChunkBatchSize_ = 0;
        uint64_t sendRecipeBatchSize_ = 0;

        // for security channel encryption
        CryptoPrimitive* cryptoObj_;
        EVP_MD_CTX* mdCtx_;
        EVP_CIPHER_CTX* cipherCtx_;

        // the sender buffer
        SendMsgBuffer_t sendPlainChunkBuf_;
        SendMsgBuffer_t sendCipherChunkBuf_;
        SendMsgBuffer_t sendPlainRecipeBuf_;
        MessageQueue<Data_t>* inputMQ_;
        
        // for chunk encryption
        AbsDAE* absDaEObj_;

        double totalTime_ = 0;
        uint64_t chunkBatchNum_ = 0;
        uint64_t recipeBatchNum_ = 0;

        /**
         * @brief send the file recipes to the cloud
         * 
         * @param recipeBuf the recipe buffer
         */
        void SendRecipes(SendMsgBuffer_t* recipeBuf);

        /**
         * @brief send the chunks to the cloud
         * 
         * @param chunkBuf the chunk buffer
         */
        void SendChunks(SendMsgBuffer_t* chunkBuf);

        /**
         * @brief process a chunk batch
         * 
         */
        void ProcessChunkBatch();

        /**
         * @brief process the recipe end 
         * 
         * @param recipeHead the ptr to the recipe end
         */
        void ProcessRecipeEnd(FileRecipeHead_t& recipeHead);

        /**
         * @brief generate the recipe from the ciphertext chunks
         * 
         * @param sendCipherChunkBuf the buffer to send the cipher chunks
         * @param sendPlainRecipeBuf the buffer to send the plain recipe
         */
        void GenerateRecipe(SendMsgBuffer_t* sendCipherChunkBuf,
            SendMsgBuffer_t* sendPlainRecipeBuf);

        /**
         * @brief add the chunk to the buffer
         * 
         * @param sendPlainChunkBuf the buffer for sending plain chunks
         * @param inputChunk the input chunk
         */
        void AddChunkToPlainBuffer(SendMsgBuffer_t* sendPlainChunkBuf, Chunk_t& inputChunk);

    public:
        /**
         * @brief Construct a new DataSender object
         * 
         * @param serverChannel the server connetion channel
         * @param absDaEObj  the ptr to the absDaEObj
         */
        DataSender(SSLConnection* serverChannel, AbsDAE* absDaEObj);

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
         * @brief send the upload login with the file name hash
         * 
         * @param fileNameHash the hash of the file name
         */
        void UploadLogin(uint8_t* fileNameHash);
        
        /**
         * @brief Set the Connection Record object
         * 
         * @param conChannelRecord the connection record
         */
        void SetConnectionRecord(pair<int, SSL*> conChannelRecord) {
            conChannelRecord_ = conChannelRecord;
        }

        /**
         * @brief Set the input MQ object
         * 
         * @param inputMQ the prt to the input MQ
         */
        void SetInputMQ(MessageQueue<Data_t>* inputMQ) {
            inputMQ_ = inputMQ;
            return ;
        }
};

#endif