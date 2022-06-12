/**
 * @file ecallRecvDecoder.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of enclave-based recv decoder
 * @version 0.1
 * @date 2021-03-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/ecallRecvDecoder.h"

/**
 * @brief Construct a new EcallRecvDecoder object
 * 
 */
EcallRecvDecoder::EcallRecvDecoder() {
    cryptoObj_ = new EcallCrypto(CIPHER_TYPE, HASH_TYPE);
    Enclave::Logging(myName_.c_str(), "init the RecvDecoder.\n");
}

/**
 * @brief Destroy the Ecall Recv Decoder object
 * 
 */
EcallRecvDecoder::~EcallRecvDecoder() {
    delete(cryptoObj_);
}

/**
 * @brief process a batch of recipes and write chunk to the outside buffer
 * 
 * @param recipeBuffer the pointer to the recipe buffer
 * @param recipeNum the input recipe buffer
 * @param resOutSGX the pointer to the out-enclave var
 * 
 * @return size_t the size of the sended buffer
 */
void EcallRecvDecoder::ProcRecipeBatch(uint8_t* recipeBuffer, size_t recipeNum, 
    ResOutSGX_t* resOutSGX) {
    // out-enclave info
    ReqContainer_t* reqContainer = (ReqContainer_t*)resOutSGX->reqContainer;
    uint8_t* idBuffer = reqContainer->idBuffer;
    uint8_t** containerArray = reqContainer->containerArray;
    SendMsgBuffer_t* sendChunkBuf = resOutSGX->sendChunkBuf;

    // in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)resOutSGX->sgxClient;
    SendMsgBuffer_t* restoreChunkBuf = &sgxClient->_restoreChunkBuffer;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;
    uint8_t* sessionKey = sgxClient->_sessionKey;
    uint8_t* masterKey = sgxClient->_masterKey;

    string tmpContainerIDStr;
    unordered_map<string, uint32_t> tmpContainerMap;
    tmpContainerMap.reserve(CONTAINER_CAPPING_VALUE);
    EnclaveRecipeEntry_t tmpEnclaveRecipeEntry;

    // decrypt the recipe file
    cryptoObj_->DecryptWithKey(cipherCtx, recipeBuffer, recipeNum * sizeof(RecipeEntry_t),
        masterKey, sgxClient->_plainRecipeBuffer);
    RecipeEntry_t* tmpRecipeEntry;
    tmpRecipeEntry = (RecipeEntry_t*)sgxClient->_plainRecipeBuffer;

    for (size_t i = 0; i < recipeNum; i++) {
        // parse the recipe entry one-by-one
        tmpContainerIDStr.assign((char*)tmpRecipeEntry->containerName, CONTAINER_ID_LENGTH);
        tmpEnclaveRecipeEntry.offset = tmpRecipeEntry->offset;
        tmpEnclaveRecipeEntry.length = tmpRecipeEntry->length;

        auto findResult = tmpContainerMap.find(tmpContainerIDStr);
        if (findResult == tmpContainerMap.end()) {
            // this is a unique container entry, it does not exist in current local index
            tmpEnclaveRecipeEntry.containerID = reqContainer->idNum;
            tmpContainerMap[tmpContainerIDStr] = reqContainer->idNum;
            memcpy(idBuffer + reqContainer->idNum * CONTAINER_ID_LENGTH, 
                tmpContainerIDStr.c_str(), CONTAINER_ID_LENGTH);
            reqContainer->idNum++;
        } else {
            // this is a duplicate container entry, using existing result.
            tmpEnclaveRecipeEntry.containerID = findResult->second;
        }
        sgxClient->_enclaveRecipeBuffer.push_back(tmpEnclaveRecipeEntry);

        // judge whether reach the capping value 
        if (reqContainer->idNum == CONTAINER_CAPPING_VALUE) {
            // start to let outside application to fetch the container data
            Ocall_GetReqContainers(resOutSGX->outClient);

            // read chunk from the encrypted container buffer, 
            // write the chunk to the outside buffer
            for (size_t idx = 0; idx < sgxClient->_enclaveRecipeBuffer.size(); idx++) {
                uint32_t containerID = sgxClient->_enclaveRecipeBuffer[idx].containerID;
                uint32_t offset = sgxClient->_enclaveRecipeBuffer[idx].offset;
                uint32_t chunkSize = sgxClient->_enclaveRecipeBuffer[idx].length;
                uint8_t* chunkBuffer = containerArray[containerID] + offset;
                this->RecoverOneChunk(chunkBuffer, chunkSize, restoreChunkBuf, cipherCtx);
                if (restoreChunkBuf->header->currentItemNum % 
                    Enclave::sendChunkBatchSize_ == 0) {
                    cryptoObj_->SessionKeyEnc(cipherCtx, restoreChunkBuf->dataBuffer,
                        restoreChunkBuf->header->dataSize, sessionKey, sendChunkBuf->dataBuffer);
                    
                    // copy the header to the send buffer
                    restoreChunkBuf->header->messageType = SERVER_RESTORE_CHUNK;
                    memcpy(sendChunkBuf->header, restoreChunkBuf->header, sizeof(NetworkHead_t));
                    Ocall_SendRestoreData(resOutSGX->outClient);

                    restoreChunkBuf->header->dataSize = 0;
                    restoreChunkBuf->header->currentItemNum = 0;
                }
            }

            // reset 
            reqContainer->idNum = 0;
            tmpContainerMap.clear();
            sgxClient->_enclaveRecipeBuffer.clear();
        }
        tmpRecipeEntry++;
    }
    return ;
}

/**
 * @brief process the tail batch of recipes
 * 
 * @param resOutSGX the pointer to the out-enclave var
 */
void EcallRecvDecoder::ProcRecipeTailBatch(ResOutSGX_t* resOutSGX) {
    // out-enclave info
    ReqContainer_t* reqContainer = (ReqContainer_t*)resOutSGX->reqContainer;
    uint8_t** containerArray = reqContainer->containerArray;
    SendMsgBuffer_t* sendChunkBuf = resOutSGX->sendChunkBuf;

    // in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)resOutSGX->sgxClient;
    SendMsgBuffer_t* restoreChunkBuf = &sgxClient->_restoreChunkBuffer;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;
    uint8_t* sessionKey = sgxClient->_sessionKey;

    if (sgxClient->_enclaveRecipeBuffer.size() != 0) {
        // start to let outside application to fetch the container data
        Ocall_GetReqContainers(resOutSGX->outClient);

        uint32_t remainChunkNum = sgxClient->_enclaveRecipeBuffer.size();
        bool endFlag = 0;
        for (size_t idx = 0; idx < sgxClient->_enclaveRecipeBuffer.size(); idx++) {
            uint32_t containerID = sgxClient->_enclaveRecipeBuffer[idx].containerID;
            uint32_t offset = sgxClient->_enclaveRecipeBuffer[idx].offset;
            uint32_t chunkSize = sgxClient->_enclaveRecipeBuffer[idx].length;
            uint8_t* chunkBuffer = containerArray[containerID] + offset;
            this->RecoverOneChunk(chunkBuffer, chunkSize, restoreChunkBuf, 
                cipherCtx);
            remainChunkNum--;
            if (remainChunkNum == 0) {
                // this is the last batch of chunks;
                endFlag = 1;
            }
            if ((restoreChunkBuf->header->currentItemNum % 
                Enclave::sendChunkBatchSize_ == 0) || endFlag) {
                cryptoObj_->SessionKeyEnc(cipherCtx, restoreChunkBuf->dataBuffer,
                    restoreChunkBuf->header->dataSize, sessionKey, 
                    sendChunkBuf->dataBuffer);

                // copy the header to the send buffer
                if (endFlag == 1) {
                    restoreChunkBuf->header->messageType = SERVER_RESTORE_FINAL;
                } else {
                    restoreChunkBuf->header->messageType = SERVER_RESTORE_CHUNK;
                }
                memcpy(sendChunkBuf->header, restoreChunkBuf->header, sizeof(NetworkHead_t));
                Ocall_SendRestoreData(resOutSGX->outClient);

                restoreChunkBuf->header->dataSize = 0;
                restoreChunkBuf->header->currentItemNum = 0;
            }
        }
    } else {
        cryptoObj_->SessionKeyEnc(cipherCtx, restoreChunkBuf->dataBuffer,
            restoreChunkBuf->header->dataSize, sessionKey,
            sendChunkBuf->dataBuffer);

        // copy the header to the send buffer
        restoreChunkBuf->header->messageType = SERVER_RESTORE_FINAL;
        memcpy(sendChunkBuf->header, restoreChunkBuf->header, sizeof(NetworkHead_t));
        Ocall_SendRestoreData(resOutSGX->outClient);

        restoreChunkBuf->header->currentItemNum = 0;
        restoreChunkBuf->header->dataSize = 0;
    }
    return ;
}

/**
 * @brief recover a chunk
 * 
 * @param chunkBuffer the chunk buffer
 * @param chunkSize the chunk size
 * @param restoreChunkBuf the restore chunk buffer
 * @param cipherCtx the pointer to the EVP cipher
 * 
 */
void EcallRecvDecoder::RecoverOneChunk(uint8_t* chunkBuffer, uint32_t chunkSize, 
    SendMsgBuffer_t* restoreChunkBuf, EVP_CIPHER_CTX* cipherCtx) {
    uint8_t* iv = chunkBuffer + chunkSize; 
    uint8_t* outputBuffer = restoreChunkBuf->dataBuffer + 
        restoreChunkBuf->header->dataSize;
    uint8_t decompressedChunk[MAX_CHUNK_SIZE];
    
    // first decrypt the chunk first
    cryptoObj_->DecryptionWithKeyIV(cipherCtx, chunkBuffer, chunkSize, 
        Enclave::enclaveKey_, decompressedChunk, iv);

    // try to decompress the chunk
    int decompressedSize = LZ4_decompress_safe((char*)decompressedChunk, 
        (char*)(outputBuffer + sizeof(uint32_t)), chunkSize, MAX_CHUNK_SIZE);
    if (decompressedSize > 0) {
        // it can do the decompression, write back the decompressed chunk size
        memcpy(outputBuffer, &decompressedSize, sizeof(uint32_t));
        restoreChunkBuf->header->dataSize += sizeof(uint32_t) + decompressedSize; 
    } else {
        // it cannot do the decompression
        memcpy(outputBuffer, &chunkSize, sizeof(uint32_t));
        memcpy(outputBuffer + sizeof(uint32_t), decompressedChunk, chunkSize);
        restoreChunkBuf->header->dataSize += sizeof(uint32_t) + chunkSize;
    }

    restoreChunkBuf->header->currentItemNum++;
    return ;
}