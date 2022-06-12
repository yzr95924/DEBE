/**
 * @file mleDaE.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of MLE
 * @version 0.1
 * @date 2021-05-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/mleDaE.h"

/**
 * @brief Destroy the MLEDaE object
 * 
 */
MLEDAE::MLEDAE(uint8_t* fileNameHash) : AbsDAE(fileNameHash) {
    tool::Logging(myName_.c_str(), "init the CE-DAE.\n");
}

/**
 * @brief Destroy the MLEDaE object
 * 
 */
MLEDAE::~MLEDAE() {
}

/**
 * @brief process one segment of chunk
 * 
 * @param sendPlainChunkBuf the buffer to the plaintext chunk
 * @param sendCipherChunkBuf the buffer to the ciphertext chunk
 * @param sendRecipeBuf the buffer for the recipe <only store the key>
 */
void MLEDAE::ProcessBatchChunk(SendMsgBuffer_t* sendPlainChunkBuf,
    SendMsgBuffer_t* sendCipherChunkBuf, SendMsgBuffer_t* sendRecipeBuf) {
    uint32_t chunkNum = sendPlainChunkBuf->header->currentItemNum;
    size_t offset = 0;

    uint32_t chunkSize;
    uint8_t* chunkData;
    KeyRecipeEntry_t* tmpRecipe = (KeyRecipeEntry_t*) (sendRecipeBuf->dataBuffer + 
        sendRecipeBuf->header->dataSize);
    uint8_t key[CHUNK_HASH_SIZE] = {0};
    for (uint32_t i = 0; i < chunkNum; i++) {
        memcpy(&chunkSize, sendPlainChunkBuf->dataBuffer + offset, 
            sizeof(uint32_t));
        offset += sizeof(uint32_t);
        chunkData = sendPlainChunkBuf->dataBuffer + offset;
        offset += chunkSize;

        // compute the chunk hash
        cryptoObj_->GenerateHash(mdCtx_, chunkData, chunkSize, key);
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

    return ;
}

/**
 * @brief close the connection with key manager
 * 
 */
void MLEDAE::CloseKMConnection() {
    return ; // does not need to anything
}