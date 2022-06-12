/**
 * @file clientVar.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the class to store the variable related to a client in the outside the enclave
 * @version 0.1
 * @date 2021-04-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef CLIENT_VAR_H
#define CLIENT_VAR_H

#include "configure.h"
#include "messageQueue.h"
#include "sslConnection.h"
#include "cryptoPrimitive.h"
#include "readCache.h"

extern Configure config;

class ClientVar {
    private:
        string myName_ = "ClientVar";
        int optType_; // the operation type (upload / download)
        uint64_t sendChunkBatchSize_;
        uint64_t sendRecipeBatchSize_;
        string recipePath_;

        string keyRecipeSuffix_ = "-key";

        /**
         * @brief init the upload buffer
         * 
         */
        void InitUploadBuffer();

        /**
         * @brief destory the upload buffer
         * 
         */
        void DestoryUploadBuffer();

        /**
         * @brief init the restore buffer
         * 
         */
        void InitRestoreBuffer();

        /**
         * @brief destory the restore buffer
         * 
         */
        void DestoryRestoreBuffer();
    public:
        uint32_t _clientID;
        // for crypto
        EVP_MD_CTX* _mdCtx;
        EVP_CIPHER_CTX* _cipherCtx;

        // for handling file recipe
        ofstream _recipeWriteHandler;
        ifstream _recipeReadHandler;

        // for handling key recipe
        ofstream _keyRecipeWriteHandler;

        // upload buffer parameters
        Container_t _curContainer;
        MessageQueue<Container_t>* _inputMQ;
        SendMsgBuffer_t _recvChunkBuf;
        Recipe_t _inRecipe;

        // restore buffer parameters
        uint8_t* _readRecipeBuf;
        ReqContainer_t _reqContainer;
        ReadCache* _containerCache;
        SendMsgBuffer_t _sendChunkBuf;
        vector<EnclaveRecipeEntry_t> _enclaveRecipeBuffer;

        SSL* _clientSSL; // connection

        // upload logical data size
        uint64_t _uploadDataSize = 0; 

        /**
         * @brief Construct a new Client Var object
         * 
         * @param clientID the client ID
         * @param clientSSL the client SSL
         * @param optType the operation type (upload / download)
         * @param recipePath the file recipe path
         */
        ClientVar(uint32_t clientID, SSL* clientSSL, 
            int optType, string& recipePath);

        /**
         * @brief Destroy the Client Var object
         * 
         */
        ~ClientVar();
};

#endif