/**
 * @file dataSender.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of data sender
 * @version 0.1
 * @date 2021-01-22
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/dataSender.h"

/**
 * @brief Construct a new DataSender object
 * 
 * @param dataSecureChannel the pointer to the dataSec
 */
DataSender::DataSender(SSLConnection* dataSecureChannel) {
    // set up the configuration
    clientID_ = config.GetClientID();
    sendChunkBatchSize_ = config.GetSendChunkBatchSize();
    dataSecureChannel_ = dataSecureChannel;
    
    // init the send chunk buffer: header + <chunkSize, chunk content>
    sendChunkBuf_.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) +
        sendChunkBatchSize_ * sizeof(Chunk_t));
    sendChunkBuf_.header = (NetworkHead_t*) sendChunkBuf_.sendBuffer;
    sendChunkBuf_.header->clientID = clientID_;
    sendChunkBuf_.header->currentItemNum = 0;
    sendChunkBuf_.header->dataSize = 0;
    sendChunkBuf_.dataBuffer = sendChunkBuf_.sendBuffer + sizeof(NetworkHead_t);

    sendEncBuffer_.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) +
        sendChunkBatchSize_ * sizeof(Chunk_t));
    sendEncBuffer_.header = (NetworkHead_t*) sendEncBuffer_.sendBuffer;
    sendEncBuffer_.header->clientID = clientID_;
    sendEncBuffer_.header->currentItemNum = 0;
    sendEncBuffer_.header->dataSize = 0;
    sendEncBuffer_.dataBuffer = sendEncBuffer_.sendBuffer + sizeof(NetworkHead_t);

    // prepare the crypto tool
    cryptoObj_ = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);
    cipherCtx_ = EVP_CIPHER_CTX_new();
    mdCtx_ = EVP_MD_CTX_new();
    tool::Logging(myName_.c_str(), "init the DataSender.\n");
}

/**
 * @brief Destroy the DataSender object
 * 
 */
DataSender::~DataSender() {
    free(sendEncBuffer_.sendBuffer);
    free(sendChunkBuf_.sendBuffer);
    EVP_CIPHER_CTX_free(cipherCtx_);
    EVP_MD_CTX_free(mdCtx_);
    delete cryptoObj_;
    fprintf(stderr, "========DataSender Info========\n");
    fprintf(stderr, "total send batch num: %lu\n", batchNum_);
    fprintf(stderr, "total thread running time: %lf\n", totalTime_);
    fprintf(stderr, "===============================\n");
}

/**
 * @brief send the upload login with the master key
 * 
 * @param localSecret the client local secret
 * @param fileNameHash the hash of the file name
 */
void DataSender::UploadLogin(string localSecret, uint8_t* fileNameHash) {
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
    msgBuf.header->messageType = CLIENT_LOGIN_UPLOAD;

    memcpy(msgBuf.dataBuffer + msgBuf.header->dataSize, fileNameHash, 
        CHUNK_HASH_SIZE);
    msgBuf.header->dataSize += CHUNK_HASH_SIZE;
    cryptoObj_->SessionKeyEnc(cipherCtx_, masterKey, CHUNK_HASH_SIZE, 
        sessionKey_, msgBuf.dataBuffer + CHUNK_HASH_SIZE);
    msgBuf.header->dataSize += CHUNK_HASH_SIZE;

    // send the upload login request
    if (!dataSecureChannel_->SendData(conChannelRecord_.second, 
        msgBuf.sendBuffer, sizeof(NetworkHead_t) + msgBuf.header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the client upload login error.\n");
        exit(EXIT_FAILURE);
    }

    // wait the server to send the login response
    uint32_t recvSize = 0;
    if (!dataSecureChannel_->ReceiveData(conChannelRecord_.second, 
        msgBuf.sendBuffer, recvSize)) {
        tool::Logging(myName_.c_str(), "recv the server login response error.\n");
        exit(EXIT_FAILURE);
    }

    if (msgBuf.header->messageType == SERVER_LOGIN_RESPONSE) {
        tool::Logging(myName_.c_str(), "recv the server login response well, "
            "the server is ready to process the request.\n");
    } else {
        tool::Logging(myName_.c_str(), "server response is wrong, it is not ready.\n");
        exit(EXIT_FAILURE);
    }

    free(msgBuf.sendBuffer);
    return ;
}

/**
 * @brief the main process of DataSender
 * 
 */
void DataSender::Run() {
    bool jobDoneFlag = false;
    Data_t tmpChunk;
    struct timeval sTotalTime;
    struct timeval eTotalTime;

    tool::Logging(myName_.c_str(), "the main thread is running.\n");
    gettimeofday(&sTotalTime, NULL);
    while (true) {
        // the main loop
        if (inputMQ_->done_ && inputMQ_->IsEmpty()) {
            jobDoneFlag = true;
        }

        if (inputMQ_->Pop(tmpChunk)) {
            switch (tmpChunk.dataType) {
                case DATA_CHUNK: {
                    // this is a normal chunk
                    this->ProcessChunk(tmpChunk.chunk);
                    break;
                }
                case RECIPE_END: {
                    // this is the recipe tail
                    this->ProcessRecipeEnd(tmpChunk.recipeHead);

                    // close the connection
                    dataSecureChannel_->Finish(conChannelRecord_);
                    break;
                }
                default: {
                    tool::Logging(myName_.c_str(), "wrong data type.\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (jobDoneFlag) {
            break;
        }
    }

    gettimeofday(&eTotalTime, NULL);
    totalTime_ += tool::GetTimeDiff(sTotalTime, eTotalTime);
    tool::Logging(myName_.c_str(), "thread exit.\n");
    return ;
}

/**
 * @brief process the recipe end
 * 
 * @param recipeHead the pointer to the recipe end
 */
void DataSender::ProcessRecipeEnd(FileRecipeHead_t& recipeHead) {
    // first check the send chunk buffer
    if (sendChunkBuf_.header->currentItemNum != 0) {
        this->SendChunks();
    }

    // send the recipe end (without session encryption)
    sendChunkBuf_.header->messageType = CLIENT_UPLOAD_RECIPE_END;
    sendChunkBuf_.header->dataSize = sizeof(FileRecipeHead_t);
    memcpy(sendChunkBuf_.dataBuffer, &recipeHead,
        sizeof(FileRecipeHead_t));
    if (!dataSecureChannel_->SendData(conChannelRecord_.second,
        sendChunkBuf_.sendBuffer, 
        sizeof(NetworkHead_t) + sendChunkBuf_.header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the recipe end error.\n");
        exit(EXIT_FAILURE);
    }
    return ;
}

/**
 * @brief process a chunk
 * 
 * @param inputChunk the input chunk
 */
void DataSender::ProcessChunk(Chunk_t& inputChunk) {
    // update the send chunk buffer
    memcpy(sendChunkBuf_.dataBuffer + sendChunkBuf_.header->dataSize,
        &inputChunk.chunkSize, sizeof(uint32_t));
    sendChunkBuf_.header->dataSize += sizeof(uint32_t);
    memcpy(sendChunkBuf_.dataBuffer + sendChunkBuf_.header->dataSize,
        inputChunk.data, inputChunk.chunkSize);
    sendChunkBuf_.header->dataSize += inputChunk.chunkSize;
    sendChunkBuf_.header->currentItemNum++;

    if (sendChunkBuf_.header->currentItemNum % sendChunkBatchSize_ == 0) {
        this->SendChunks();
    }
    return ;
}

/**
 * @brief send a batch of chunks
 * 
 * @param chunkBuffer the chunk buffer
 */
void DataSender::SendChunks() {
    sendChunkBuf_.header->messageType = CLIENT_UPLOAD_CHUNK;

    // encrypt the payload with the session key
    cryptoObj_->SessionKeyEnc(cipherCtx_, sendChunkBuf_.dataBuffer,
        sendChunkBuf_.header->dataSize, sessionKey_,
        sendEncBuffer_.dataBuffer);

    memcpy(sendEncBuffer_.header, sendChunkBuf_.header, 
        sizeof(NetworkHead_t));
    
    if (!dataSecureChannel_->SendData(conChannelRecord_.second, 
        sendEncBuffer_.sendBuffer, 
        sizeof(NetworkHead_t) + sendEncBuffer_.header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the chunk batch error.\n");
        exit(EXIT_FAILURE);
    }
    
    // clear the current chunk buffer
    sendChunkBuf_.header->currentItemNum = 0;
    sendChunkBuf_.header->dataSize = 0;
    batchNum_++;

    return ;
}