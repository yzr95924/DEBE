/**
 * @file tedDaE.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2021-12-12
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/tedDaE.h"

/**
 * @brief Construct a new TEDDAE object
 * 
 */
TEDDAE::TEDDAE(uint8_t* fileNameHash) : AbsDAE(fileNameHash) {
    keyManagerChannel_ = new SSLConnection(config.GetKeyServerIP(), config.GetKeyServerPort(), 
        IN_CLIENTSIDE);
    keyChannelRecord_ = keyManagerChannel_->ConnectSSL();
    
    // init the send short hash buffer
    sendShortHashBuf_.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendChunkBatchSize_ * CHUNK_HASH_SIZE);
    sendShortHashBuf_.header = (NetworkHead_t*) sendShortHashBuf_.sendBuffer;
    sendShortHashBuf_.header->clientID = config.GetClientID();
    sendShortHashBuf_.header->currentItemNum = 0;
    sendShortHashBuf_.header->dataSize = 0;
    sendShortHashBuf_.dataBuffer = sendShortHashBuf_.sendBuffer + sizeof(NetworkHead_t);

    tool::Logging(myName_.c_str(), "init the TED-DAE.\n");
}

/**
 * @brief Destroy the TEDDAE object
 * 
 */
TEDDAE::~TEDDAE() {
    delete keyManagerChannel_;
    free(sendShortHashBuf_.sendBuffer);
}

/**
 * @brief process one segment of chunk
 * 
 * @param sendPlainChunkBuf the buffer to the plaintext chunk
 * @param sendCipherChunkBuf the buffer to the ciphertext chunk
 * @param sendRecipeBuf the buffer for the recipe <only store the key>
 */
void TEDDAE::ProcessBatchChunk(SendMsgBuffer_t* sendPlainChunkBuf,
    SendMsgBuffer_t* sendCipherChunkBuf, SendMsgBuffer_t* sendRecipeBuf) {
    uint32_t chunkNum = sendPlainChunkBuf->header->currentItemNum;
    size_t offset = 0;

    uint32_t chunkSize;
    uint8_t* chunkData;
    for (uint32_t i = 0; i < chunkNum; i++) {
        memcpy(&chunkSize, sendPlainChunkBuf->dataBuffer + offset, 
            sizeof(uint32_t));
        offset += sizeof(uint32_t);
        chunkData = sendPlainChunkBuf->dataBuffer + offset;
        offset += chunkSize;

        // compute the short hash of the chunk
        MurmurHash3_x64_128(chunkData, chunkSize, 0, 
            sendShortHashBuf_.dataBuffer + sendShortHashBuf_.header->dataSize);
        sendShortHashBuf_.header->dataSize += CHUNK_HASH_SIZE;
        sendShortHashBuf_.header->currentItemNum++;
    }

    sendShortHashBuf_.header->messageType = CLIENT_KEY_GEN;
    if (!keyManagerChannel_->SendData(keyChannelRecord_.second, sendShortHashBuf_.sendBuffer, 
        sizeof(NetworkHead_t) + sendShortHashBuf_.header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the key gen request error.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t recvSize = 0;
    if (!keyManagerChannel_->ReceiveData(keyChannelRecord_.second, sendShortHashBuf_.sendBuffer, 
        recvSize)) {
        tool::Logging(myName_.c_str(), "recv the key gen response error.\n");
        exit(EXIT_FAILURE);
    }
    if (sendShortHashBuf_.header->messageType != KEY_MANAGER_KEY_GEN_REPLY) {
        tool::Logging(myName_.c_str(), "wrong key manager reply type.\n");
        exit(EXIT_FAILURE);
    }

    offset = 0;
    uint8_t key[CHUNK_HASH_SIZE] = {0};
    uint8_t keySeed[CHUNK_HASH_SIZE * 2] = {0}; 
    KeyRecipeEntry_t* tmpRecipe = (KeyRecipeEntry_t*) (sendRecipeBuf->dataBuffer + 
        sendRecipeBuf->header->dataSize);
    for (uint32_t i = 0; i < chunkNum; i++) {
        memcpy(&chunkSize, sendPlainChunkBuf->dataBuffer + offset, 
            sizeof(uint32_t));
        offset += sizeof(uint32_t);
        chunkData = sendPlainChunkBuf->dataBuffer + offset;
        offset += chunkSize;

        // compute the chunk hash
        cryptoObj_->GenerateHash(mdCtx_, chunkData, chunkSize, keySeed);
        memcpy(keySeed + CHUNK_HASH_SIZE, sendShortHashBuf_.dataBuffer + i * CHUNK_HASH_SIZE, 
            CHUNK_HASH_SIZE);
        cryptoObj_->GenerateHash(mdCtx_, keySeed, CHUNK_HASH_SIZE * 2, key);
        keyRecipeFile_.write((char*)key, CHUNK_HASH_SIZE);
        memcpy(tmpRecipe->key, key, CHUNK_HASH_SIZE);
        tmpRecipe++;

        // encrypt this chunk to the cipher chunk buffer
        memcpy(sendCipherChunkBuf->dataBuffer + sendCipherChunkBuf->header->dataSize, 
            &chunkSize, sizeof(uint32_t));
        sendCipherChunkBuf->header->dataSize += sizeof(uint32_t);
        cryptoObj_->EncryptWithKey(cipherCtx_, chunkData, chunkSize, key, 
            sendCipherChunkBuf->dataBuffer + sendCipherChunkBuf->header->dataSize);
        sendCipherChunkBuf->header->dataSize += chunkSize;
        sendCipherChunkBuf->header->currentItemNum++;
    }

    // reset the sendShortHashBuf
    sendShortHashBuf_.header->dataSize = 0;
    sendShortHashBuf_.header->currentItemNum = 0;

    return ;
}

/**
 * @brief close the connection with key manager
 * 
 */
void TEDDAE::CloseKMConnection() {
    keyManagerChannel_->Finish(keyChannelRecord_);
    return ;
}