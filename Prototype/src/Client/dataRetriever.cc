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

extern Configure config;

/**
 * @brief Construct a new Data Retriever object
 * 
 * @param dataSecureChannel the pointer to the data secure channel
 */
DataRetriever::DataRetriever(SSLConnection* dataSecureChannel) {
    // set up the configuration
    clientID_ = config.GetClientID();
    recvChunkBatchSize_ = config.GetSendChunkBatchSize();
    cryptoObj_ = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);
    dataSecureChannel_ = dataSecureChannel;

    // for received data buffer
    recvChunkBuf_.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        recvChunkBatchSize_ * sizeof(Chunk_t));
    recvChunkBuf_.header = (NetworkHead_t*) recvChunkBuf_.sendBuffer;
    recvChunkBuf_.header->clientID = clientID_;
    recvChunkBuf_.header->currentItemNum = 0;
    recvChunkBuf_.header->dataSize = 0;
    recvChunkBuf_.dataBuffer = recvChunkBuf_.sendBuffer + sizeof(NetworkHead_t);

    // for decryption 
    decBuffer_ = (uint8_t*) malloc(recvChunkBatchSize_ * sizeof(Chunk_t));
    
    cipherCtx_ = EVP_CIPHER_CTX_new();
    mdCtx_ = EVP_MD_CTX_new();
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
    free(decBuffer_);
    free(recvChunkBuf_.sendBuffer);
    fprintf(stderr, "========DataRetriever Info========\n");
    fprintf(stderr, "total thread running time: %lf\n", totalTime_);
    fprintf(stderr, "total recv chunk num: %lu\n", _totalRecvChunkNum);
    fprintf(stderr, "total recv data size: %lu\n", _totalRecvDataSize);
    fprintf(stderr, "==================================\n");
}

/**
 * @brief init the restore login with the server 
 * 
 * @param localSecret the client local secret
 * @param fileNameHash the hash of the file name
 */
void DataRetriever::RestoreLogin(string localSecret, uint8_t* fileNameHash) {
    // generate the hash of the master key
    uint8_t masterKey[CHUNK_HASH_SIZE];
    cryptoObj_->GenerateHash(mdCtx_, (uint8_t*)&localSecret[0], localSecret.size(),
        masterKey);

    // header + fileNameHash + Enc(masterKey)
    SendMsgBuffer_t msgBuf;
    msgBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        CHUNK_HASH_SIZE + CHUNK_HASH_SIZE);
    msgBuf.header = (NetworkHead_t*) msgBuf.sendBuffer;
    msgBuf.header->clientID = clientID_;
    msgBuf.header->dataSize = 0;
    msgBuf.dataBuffer = msgBuf.sendBuffer + sizeof(NetworkHead_t);
    msgBuf.header->messageType = CLIENT_LOGIN_DOWNLOAD;

    memcpy(msgBuf.dataBuffer + msgBuf.header->dataSize, fileNameHash,
        CHUNK_HASH_SIZE);
    msgBuf.header->dataSize += CHUNK_HASH_SIZE;
    cryptoObj_->SessionKeyEnc(cipherCtx_, masterKey, CHUNK_HASH_SIZE, 
        sessionKey_, msgBuf.dataBuffer + CHUNK_HASH_SIZE);
    msgBuf.header->dataSize += CHUNK_HASH_SIZE;
    
    // send the restore login request
    if (!dataSecureChannel_->SendData(conChannelRecord_.second,
        msgBuf.sendBuffer, sizeof(NetworkHead_t) + msgBuf.header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the client restore login error.\n");
        exit(EXIT_FAILURE);
    }

    // wait the server to send the login response 
    uint32_t recvSize = 0;
    if (!dataSecureChannel_->ReceiveData(conChannelRecord_.second,
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
    tool::Logging(myName_.c_str(), "total chunk num: %lu\n", fileRecipeHead_.totalChunkNum);
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
    if (!dataSecureChannel_->SendData(conChannelRecord_.second,
        recvChunkBuf_.sendBuffer, sizeof(NetworkHead_t))) {
        tool::Logging(myName_.c_str(), "send the ready message error.\n");
        exit(EXIT_FAILURE);
    }

    // start to receive the data from the server 
    tool::Logging(myName_.c_str(), "start to recv the data from the server.\n");
    while (true) {
        if (!dataSecureChannel_->ReceiveData(conChannelRecord_.second, 
            recvChunkBuf_.sendBuffer, recvSize)) {
            tool::Logging(myName_.c_str(), "recv the data from the server error.\n");
            exit(EXIT_FAILURE);
        } else {
            switch (recvChunkBuf_.header->messageType) {
                case SERVER_RESTORE_CHUNK:
                    this->ProcessChunkBatch();
                    break;
                case SERVER_RESTORE_FINAL:
                    this->ProcessChunkBatch();
                    // close the connection
                    dataSecureChannel_->Finish(conChannelRecord_);
                    // set the done flag
                    outputMQ_->done_ = true;
                    jobDoneFlag = true;
                    break;
                default:
                    tool::Logging(myName_.c_str(), "wrong message type.\n");
                    exit(EXIT_FAILURE); 
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

    // decrypt the recv message
    cryptoObj_->SessionKeyDec(cipherCtx_, recvChunkBuf_.dataBuffer,
        recvChunkBuf_.header->dataSize, sessionKey_,
        decBuffer_);

    size_t offset = 0;
    for (size_t i = 0; i < chunkNum; i++) {
        // copy the data into the chunk
        memcpy(&tmpChunk.chunkSize, decBuffer_ + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(tmpChunk.data, decBuffer_ + offset, tmpChunk.chunkSize);
        offset += tmpChunk.chunkSize;

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