/**
 * @file ecallStorage.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of storage core inside the enclave 
 * @version 0.1
 * @date 2020-12-16
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/ecallStorage.h"

/**
 * @brief Construct a new Ecall Storage Core object
 * 
 */
EcallStorageCore::EcallStorageCore() {
    Enclave::Logging(myName_.c_str(), "init the StorageCore.\n");
}

 /**
 * @brief Destroy the Ecall Storage Core object
 * 
 */
EcallStorageCore::~EcallStorageCore() {
    Enclave::Logging(myName_.c_str(), "========StorageCore Info========\n");
    Enclave::Logging(myName_.c_str(), "write the data size: %lu\n", writtenDataSize_);
    Enclave::Logging(myName_.c_str(), "write chunk num: %lu\n", writtenChunkNum_);
    Enclave::Logging(myName_.c_str(), "================================\n");
}

/**
 * @brief save the chunk to the storage serve
 * 
 * @param chunkData the chunk data buffer
 * @param chunkSize the chunk size
 * @param chunkAddr the chunk address (return)
 * @param sgxClient the current client
 * @param upOutSGX the pointer to outside SGX buffer
 */
void EcallStorageCore::SaveChunk(char* chunkData, uint32_t chunkSize,
    RecipeEntry_t* chunkAddr, UpOutSGX_t* upOutSGX) {
    // assign a chunk length
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    InContainer* inContainer = &sgxClient->_inContainer;
    Container_t* outContainer = upOutSGX->curContainer;

    chunkAddr->length = chunkSize;
    uint32_t saveOffset = inContainer->curSize;
    uint32_t writeOffset = saveOffset;

    if (CRYPTO_BLOCK_SIZE + chunkSize + saveOffset < MAX_CONTAINER_SIZE) {
        // current container can store this chunk
        // copy data to this container
        memcpy(inContainer->buf + writeOffset, chunkData, chunkSize);
        writeOffset += chunkSize;
        memcpy(inContainer->buf + writeOffset, sgxClient->_iv, CRYPTO_BLOCK_SIZE);
        memcpy(chunkAddr->containerName, outContainer->containerID, CONTAINER_ID_LENGTH);
    } else {
        // current container cannot store this chunk, write this container to the outside buffer
        // create a new container for this new chunk
        memcpy(outContainer->body, inContainer->buf, inContainer->curSize);
        outContainer->currentSize = inContainer->curSize;
        inContainer->curSize = 0;
        Ocall_WriteContainer(upOutSGX->outClient);
        // reset this container during the ocall

        saveOffset = 0;
        writeOffset = saveOffset;
        memcpy(inContainer->buf + writeOffset, chunkData, chunkSize);
        writeOffset += chunkSize;
        memcpy(inContainer->buf + writeOffset, sgxClient->_iv, CRYPTO_BLOCK_SIZE);
        memcpy(chunkAddr->containerName, outContainer->containerID, CONTAINER_ID_LENGTH);
    }

    inContainer->curSize += chunkSize;
    inContainer->curSize += CRYPTO_BLOCK_SIZE;

    chunkAddr->offset = saveOffset;

    writtenDataSize_ += chunkSize;
    writtenChunkNum_++;

    return ;
}