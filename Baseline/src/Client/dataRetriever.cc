/**
 * @file dataRetriever.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of data retriever
 * @version 0.1
 * @date 2021-02-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/dataRetriever.h"

/**
 * @brief Construct a new Data Retriever object
 * 
 * @param serverChannel the pointer to the server communication channel
 * @param methodType the method type
 */
DataRetriever::DataRetriever(SSLConnection* serverChannel, int methodType, 
    uint8_t* fileNameHash) {
    // set up the configuration
    clientID_ = config.GetClientID();
    recvChunkBatchSize_ = config.GetSendChunkBatchSize();
    cryptoObj_ = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);
    serverChannel_ = serverChannel;
    methodType_ = methodType;

    // for received data buffer
    recvChunkBuf_.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        recvChunkBatchSize_ * sizeof(Chunk_t));
    recvChunkBuf_.header = (NetworkHead_t*) recvChunkBuf_.sendBuffer;
    recvChunkBuf_.header->clientID = clientID_;
    recvChunkBuf_.header->currentItemNum = 0;
    recvChunkBuf_.header->dataSize = 0;
    recvChunkBuf_.dataBuffer = recvChunkBuf_.sendBuffer + sizeof(NetworkHead_t);

    mdCtx_ = EVP_MD_CTX_new();
    cipherCtx_ = EVP_CIPHER_CTX_new();

    char keyRecipeNameBuffer[CHUNK_HASH_SIZE * 2 + 1];
    for (size_t i = 0; i < CHUNK_HASH_SIZE; i++) {
        sprintf(keyRecipeNameBuffer + 2 * i, "%02X", fileNameHash[i]);
    }
    keyRecipeNameStr_.assign(keyRecipeNameBuffer, CHUNK_HASH_SIZE * 2);
    keyRecipeNameStr_ = config.GetRecipeRootPath() + keyRecipeNameStr_ + keyRecipeLocalSuffix_;

    keyRecipeFile_.open(keyRecipeNameStr_.c_str(), ios_base::in);
    if (!keyRecipeFile_.is_open()) {
        tool::Logging(myName_.c_str(), "cannot open the key recipe: %s.\n", 
            keyRecipeNameStr_.c_str());
        exit(EXIT_FAILURE);
    }

    readKeyBuffer_ = (uint8_t*) malloc(recvChunkBatchSize_ * CHUNK_HASH_SIZE);

    tool::Logging(myName_.c_str(), "init the DataRetriever.\n");
}

/**
* @brief Destroy the Data Retriever object
* 
*/
DataRetriever::~DataRetriever() {
    delete cryptoObj_;
    EVP_CIPHER_CTX_free(cipherCtx_);
    EVP_MD_CTX_free(mdCtx_);
    free(recvChunkBuf_.sendBuffer);
    free(readKeyBuffer_);
    keyRecipeFile_.close();
    fprintf(stderr, "========DataRetriever Info========\n");
    fprintf(stderr, "total thread running time: %lf\n", totalTime_);
    fprintf(stderr, "total recv chunk num: %lu\n", _totalRecvChunkNum);
    fprintf(stderr, "total recv data size: %lu\n", _totalRecvDataSize);
    fprintf(stderr, "==================================\n");
}

/**
 * @brief init the restore login with the server
 * 
 * @param fileNameHash the hash of the file name
 */
void DataRetriever::RestoreLogin(uint8_t* fileNameHash) {
    // header + fileNameHash
    SendMsgBuffer_t msgBuf;
    msgBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        CHUNK_HASH_SIZE);
    msgBuf.header = (NetworkHead_t*) msgBuf.sendBuffer;
    msgBuf.header->clientID = clientID_;
    msgBuf.header->dataSize = 0;
    msgBuf.dataBuffer = msgBuf.sendBuffer + sizeof(NetworkHead_t);
    msgBuf.header->messageType = CLIENT_LOGIN_DOWNLOAD;

    memcpy(msgBuf.dataBuffer + msgBuf.header->dataSize, fileNameHash,
        CHUNK_HASH_SIZE);
    msgBuf.header->dataSize += CHUNK_HASH_SIZE;

    // send the restore login request
    if (!serverChannel_->SendData(conChannelRecord_.second,
        msgBuf.sendBuffer, sizeof(NetworkHead_t) + msgBuf.header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the client restore login error.n");
        exit(EXIT_FAILURE);
    }

    // wait the server to send the login response
    uint32_t recvSize = 0;
    if (!serverChannel_->ReceiveData(conChannelRecord_.second,
        msgBuf.sendBuffer, recvSize)) {
        tool::Logging(myName_.c_str(), "recv the server login response error.\n");
        exit(EXIT_FAILURE);
    }

    switch (msgBuf.header->messageType) {
        case SERVER_FILE_NON_EXIST: {
            tool::Logging(myName_.c_str(), "the request file not exist.\n");
            exit(EXIT_FAILURE);
        }
        case SERVER_LOGIN_RESPONSE: {
            tool::Logging(myName_.c_str(), "recv the server login response well, "
                "the server is ready to process the request.\n");
            break;
        }
        default: {
            tool::Logging(myName_.c_str(), "server response is wrong, it is not ready.\n");
            exit(EXIT_FAILURE);
        }
    }

    memcpy(&fileRecipeHead_, msgBuf.dataBuffer, sizeof(FileRecipeHead_t));
    tool::Logging(myName_.c_str(), "total check num: %lu\n", fileRecipeHead_.totalChunkNum);
    tool::Logging(myName_.c_str(), "file size: %lu\n", fileRecipeHead_.fileSize);

    free(msgBuf.sendBuffer);
    return ;
}

/**
 * @brief the main process
 * 
 */
void DataRetriever::Run() {
    bool jobDoneFlag = false;
    struct timeval sTotalTime;
    struct timeval eTotalTime;

    tool::Logging(myName_.c_str(), "the main thread is running.\n");
    gettimeofday(&sTotalTime, NULL);
    uint32_t recvSize = 0;

    // let the server know, it is ready to recv data
    recvChunkBuf_.header->messageType = CLIENT_RESTORE_READY;
    if (!serverChannel_->SendData(conChannelRecord_.second,
        recvChunkBuf_.sendBuffer, sizeof(NetworkHead_t))) {
        tool::Logging(myName_.c_str(), "send the ready message error.\n");
        exit(EXIT_FAILURE);
    }

    // start to receive the data from the server
    tool::Logging(myName_.c_str(), "start to recv the data from the server.\n");
    while (true) {
        // wait the recipe data/ restore chunk from the server
        if (!serverChannel_->ReceiveData(conChannelRecord_.second,
            recvChunkBuf_.sendBuffer, recvSize)) {
            tool::Logging(myName_.c_str(), "recv the data from the server error.\n");
            exit(EXIT_FAILURE);
        } else {
            switch (recvChunkBuf_.header->messageType) {
                case SERVER_RESTORE_CHUNK: {
                    this->ProcessChunkBatch();
                    break;
                }
                case SERVER_RESTORE_FINAL: {
                    this->ProcessChunkBatch();
                    // close the connection
                    serverChannel_->Finish(conChannelRecord_);
                    // set the done flag
                    outputMQ_->done_ = true;
                    jobDoneFlag = true;
                    break;
                }
                default: {
                    tool::Logging(myName_.c_str(), "wrong recv msg type.\n");
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (jobDoneFlag) {
            break;
        }
    }

    if (_totalRecvChunkNum != fileRecipeHead_.totalChunkNum) {
        tool::Logging(myName_.c_str(), "recv chunk num: %lu, expected chunk num: %lu, cannot match.\n",
            _totalRecvChunkNum, fileRecipeHead_.totalChunkNum);
        exit(EXIT_FAILURE);
    }

    tool::Logging(myName_.c_str(), "thread exit.\n");
    gettimeofday(&eTotalTime, NULL);
    totalTime_ += tool::GetTimeDiff(sTotalTime, eTotalTime);
    return ; 
}

/**
 * @brief process a batch of chunks
 * 
 */
void DataRetriever::ProcessChunkBatch() {
    Chunk_t tmpChunk;
    uint32_t chunkNum = recvChunkBuf_.header->currentItemNum;

    keyRecipeFile_.read((char*)readKeyBuffer_, chunkNum * CHUNK_HASH_SIZE);

    size_t offset = 0;
    uint8_t* tmpKey = readKeyBuffer_;
    for (uint32_t i = 0; i < chunkNum; i++) {
        memcpy(&tmpChunk.chunkSize, recvChunkBuf_.dataBuffer + offset, 
            sizeof(uint32_t));
        offset += sizeof(uint32_t);

        if (methodType_ != PLAIN_DAE) {
            // perform decryption
            cryptoObj_->DecryptWithKey(cipherCtx_, recvChunkBuf_.dataBuffer + offset, 
                tmpChunk.chunkSize, tmpKey, tmpChunk.data);
        } else {
            // without decryption, directly copy
            memcpy(tmpChunk.data, recvChunkBuf_.dataBuffer + offset, 
                tmpChunk.chunkSize);
        }
        offset += tmpChunk.chunkSize;
        tmpKey += CHUNK_HASH_SIZE;

        // insert hte chunk to the MQ
        if (!outputMQ_->Push(tmpChunk)) {
            tool::Logging(myName_.c_str(), "insert to output MQ error.\n");
            exit(EXIT_FAILURE);            
        }

        _totalRecvDataSize += tmpChunk.chunkSize;  
        _totalRecvChunkNum++;
    }

    return ;
}