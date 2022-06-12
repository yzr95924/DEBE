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
 * @param serverChannel the server connetion channel
 * @param absDaEObj  the ptr to the absDaEObj
 */
DataSender::DataSender(SSLConnection* serverChannel, AbsDAE* absDaEObj) {
    // set up the configuration
    clientID_ = config.GetClientID();
    sendChunkBatchSize_ = config.GetSendChunkBatchSize();
    sendRecipeBatchSize_ = config.GetSendRecipeBatchSize();
    serverChannel_ = serverChannel;
    absDaEObj_ = absDaEObj;

    // init the buffer to buffer the chunk
    sendPlainChunkBuf_.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendChunkBatchSize_ * (sizeof(uint32_t) + MAX_CHUNK_SIZE));
    sendPlainChunkBuf_.header = (NetworkHead_t*) sendPlainChunkBuf_.sendBuffer;
    sendPlainChunkBuf_.header->clientID = config.GetClientID();
    sendPlainChunkBuf_.header->currentItemNum = 0;
    sendPlainChunkBuf_.header->dataSize = 0;
    sendPlainChunkBuf_.dataBuffer = sendPlainChunkBuf_.sendBuffer + sizeof(NetworkHead_t);

    // init the send chunk buffer: header + <chunkSize, chunk content>
    sendCipherChunkBuf_.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendChunkBatchSize_ * (sizeof(uint32_t) + MAX_CHUNK_SIZE));
    sendCipherChunkBuf_.header = (NetworkHead_t*) sendCipherChunkBuf_.sendBuffer;
    sendCipherChunkBuf_.header->clientID = clientID_;
    sendCipherChunkBuf_.header->currentItemNum = 0;
    sendCipherChunkBuf_.header->dataSize = 0;
    sendCipherChunkBuf_.dataBuffer = sendCipherChunkBuf_.sendBuffer + sizeof(NetworkHead_t);

    // init the send plain recipe buffer: header + <chunk fp, key>
    sendPlainRecipeBuf_.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendRecipeBatchSize_ * sizeof(KeyRecipeEntry_t));
    sendPlainRecipeBuf_.header = (NetworkHead_t*) sendPlainRecipeBuf_.sendBuffer;
    sendPlainRecipeBuf_.header->clientID = clientID_;
    sendPlainRecipeBuf_.header->currentItemNum = 0;
    sendPlainRecipeBuf_.header->dataSize = 0;
    sendPlainRecipeBuf_.dataBuffer = sendPlainRecipeBuf_.sendBuffer + sizeof(NetworkHead_t);

    // crypto
    cryptoObj_ = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);
    mdCtx_ = EVP_MD_CTX_new();
    cipherCtx_ = EVP_CIPHER_CTX_new();

    // prepare the crypto tool
    tool::Logging(myName_.c_str(), "init the DataSender.\n");
}

/**
 * @brief Destroy the DataSender object
 * 
 */
DataSender::~DataSender() {
    free(sendPlainChunkBuf_.sendBuffer); 
    free(sendCipherChunkBuf_.sendBuffer); 
    free(sendPlainRecipeBuf_.sendBuffer);
    delete cryptoObj_;
    EVP_MD_CTX_free(mdCtx_);
    EVP_CIPHER_CTX_free(cipherCtx_);
    fprintf(stderr, "========DataSender Info========\n");
    fprintf(stderr, "total send chunk batch num: %lu\n", chunkBatchNum_);
    fprintf(stderr, "total send recipe batch num: %lu\n", recipeBatchNum_);
    fprintf(stderr, "total thread running time: %lf\n", totalTime_);
    fprintf(stderr, "===============================\n");
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
                    // this is normal chunk, buffer this chunk in the batch
                    this->AddChunkToPlainBuffer(&sendPlainChunkBuf_, tmpChunk.chunk);
                    if (sendPlainChunkBuf_.header->currentItemNum % sendChunkBatchSize_ == 0) {
                        // process the batch
                        this->ProcessChunkBatch();

                        // reset the batch buffer
                        sendPlainChunkBuf_.header->dataSize = 0;
                        sendPlainChunkBuf_.header->currentItemNum = 0;
                    }
                    break;
                }
                case RECIPE_END: {
                    // this is the recipe tail
                    this->ProcessRecipeEnd(tmpChunk.recipeHead);

                    // close the connection
                    serverChannel_->Finish(conChannelRecord_);
                    absDaEObj_->CloseKMConnection();
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
 * @brief send the upload login with the file name hash
 * 
 * @param fileNameHash the hash of the file name
 */
void DataSender::UploadLogin(uint8_t* fileNameHash) {
    // header + fileNameHash
    SendMsgBuffer_t msgBuf;
    msgBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        CHUNK_HASH_SIZE);
    msgBuf.header = (NetworkHead_t*) msgBuf.sendBuffer;
    msgBuf.header->clientID = clientID_;
    msgBuf.header->dataSize = 0;
    msgBuf.dataBuffer = msgBuf.sendBuffer + sizeof(NetworkHead_t);
    msgBuf.header->messageType = CLIENT_LOGIN_UPLOAD;

    memcpy(msgBuf.dataBuffer + msgBuf.header->dataSize, fileNameHash, 
        CHUNK_HASH_SIZE);
    msgBuf.header->dataSize += CHUNK_HASH_SIZE;

    // send the upload login request
    if (!serverChannel_->SendData(conChannelRecord_.second, 
        msgBuf.sendBuffer, sizeof(NetworkHead_t) + msgBuf.header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the client upload login error.\n");
        exit(EXIT_FAILURE);
    }

    // wait the server to send the login response
    uint32_t recvSize = 0;
    if (!serverChannel_->ReceiveData(conChannelRecord_.second, 
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
 * @brief process a chunk batch
 * 
 */
void DataSender::ProcessChunkBatch() {
    if (sendPlainChunkBuf_.header->currentItemNum != 0) {
        absDaEObj_->ProcessBatchChunk(&sendPlainChunkBuf_, &sendCipherChunkBuf_, 
            &sendPlainRecipeBuf_);
        this->GenerateRecipe(&sendCipherChunkBuf_, &sendPlainRecipeBuf_);
    }

    // check the send recipe batch size
    if (sendPlainRecipeBuf_.header->currentItemNum % sendRecipeBatchSize_ == 0) {
        this->SendRecipes(&sendPlainRecipeBuf_); // TODO: may consider encrypted by the master key
    }

    // check the send chunk batch size
    if (sendCipherChunkBuf_.header->currentItemNum % sendChunkBatchSize_ == 0) {
        this->SendChunks(&sendCipherChunkBuf_);
    }

    return ;
}

/**
 * @brief generate the recipe from the ciphertext chunks
 * 
 * @param sendCipherChunkBuf the buffer to send the cipher chunks
 * @param sendPlainRecipeBuf the buffer to send the plain recipe
 */
void DataSender::GenerateRecipe(SendMsgBuffer_t* sendCipherChunkBuf, 
    SendMsgBuffer_t* sendPlainRecipeBuf) {
    // prepare the recipe buffer
    uint32_t chunkNum = sendCipherChunkBuf->header->currentItemNum;
    uint32_t chunkSize;
    uint8_t* chunkData;
    size_t offset = 0;
    KeyRecipeEntry_t* tmpRecipe = (KeyRecipeEntry_t*) (sendPlainRecipeBuf->dataBuffer + 
        sendPlainRecipeBuf->header->dataSize);
    for (uint32_t i = 0; i < chunkNum; i++) {
        memcpy(&chunkSize, sendCipherChunkBuf->dataBuffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        chunkData = sendCipherChunkBuf->dataBuffer + offset;

        // compute the cipher chunk fp
        cryptoObj_->GenerateHash(mdCtx_, chunkData, chunkSize,
            tmpRecipe->cipherChunkHash);
        tmpRecipe++;

        offset += chunkSize;
    }
    sendPlainRecipeBuf->header->currentItemNum += chunkNum;
    sendPlainRecipeBuf->header->dataSize += chunkNum * sizeof(KeyRecipeEntry_t);

    return ;
}

/**
 * @brief send the file recipe to the cloud
 * 
 * @param recipeBuf the recipe buffer
 */
void DataSender::SendRecipes(SendMsgBuffer_t* recipeBuf) {
    recipeBuf->header->messageType = CLIENT_UPLOAD_RECIPE;
    if (!serverChannel_->SendData(conChannelRecord_.second,
        recipeBuf->sendBuffer, 
        recipeBuf->header->dataSize + sizeof(NetworkHead_t))) {
        tool::Logging(myName_.c_str(), "send the recipe batch error.\n");
        exit(EXIT_FAILURE);
    }

    // clear the current recipe buffer
    recipeBuf->header->currentItemNum = 0;
    recipeBuf->header->dataSize = 0;
    recipeBatchNum_++;

    return ;
}

/**
 * @brief send a batch of chunks
 * 
 * @param chunkBuf the chunk buffer
 */
void DataSender::SendChunks(SendMsgBuffer_t* chunkBuf) {
    chunkBuf->header->messageType = CLIENT_UPLOAD_CHUNK;
    if (!serverChannel_->SendData(conChannelRecord_.second,
        chunkBuf->sendBuffer,
        chunkBuf->header->dataSize + sizeof(NetworkHead_t))) {
        tool::Logging(myName_.c_str(), "send the chunk batch error.\n");
        exit(EXIT_FAILURE);
    }

    // clear the current chunk buffer
    chunkBuf->header->currentItemNum = 0;
    chunkBuf->header->dataSize = 0;
    chunkBatchNum_++;

    return ;
}

/**
 * @brief add the chunk to the buffer
 *
 * @param sendPlainChunkBuf the buffer for sending plain chunks 
 * @param inputChunk the input chunk
 */
void DataSender::AddChunkToPlainBuffer(SendMsgBuffer_t* sendPlainChunkBuf, Chunk_t& inputChunk) {
    // update the send chunk buffer
    memcpy(sendPlainChunkBuf_.dataBuffer + sendPlainChunkBuf_.header->dataSize,
        &inputChunk.chunkSize, sizeof(uint32_t));
    sendPlainChunkBuf_.header->dataSize += sizeof(uint32_t);
    memcpy(sendPlainChunkBuf_.dataBuffer + sendPlainChunkBuf_.header->dataSize,
        inputChunk.data, inputChunk.chunkSize);
    sendPlainChunkBuf_.header->dataSize += inputChunk.chunkSize;
    sendPlainChunkBuf_.header->currentItemNum++;
    
    return ;
}

/**
 * @brief process the recipe end
 * 
 * @param recipeHead the pointer to the recipe end
 */
void DataSender::ProcessRecipeEnd(FileRecipeHead_t& recipeHead) {
    // first check the send chunk buffer
    if (sendPlainChunkBuf_.header->currentItemNum != 0) {
        absDaEObj_->ProcessBatchChunk(&sendPlainChunkBuf_, &sendCipherChunkBuf_, 
            &sendPlainRecipeBuf_);
        this->GenerateRecipe(&sendCipherChunkBuf_, &sendPlainRecipeBuf_);
    }

    if (sendPlainRecipeBuf_.header->currentItemNum != 0) {
        this->SendRecipes(&sendPlainRecipeBuf_); // TODO: may consider encrypted by the master key
    }

    if (sendCipherChunkBuf_.header->currentItemNum != 0) {
        this->SendChunks(&sendCipherChunkBuf_);
    }

    // send the recipe end (without session encryption)
    sendCipherChunkBuf_.header->messageType = CLIENT_UPLOAD_RECIPE_END;
    sendCipherChunkBuf_.header->dataSize = sizeof(FileRecipeHead_t);
    memcpy(sendCipherChunkBuf_.dataBuffer, &recipeHead,
        sizeof(FileRecipeHead_t));
    if (!serverChannel_->SendData(conChannelRecord_.second, 
        sendCipherChunkBuf_.sendBuffer, 
        sizeof(NetworkHead_t) + sendCipherChunkBuf_.header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the recipe end error.\n");
        exit(EXIT_FAILURE);
    }
    return ;
}