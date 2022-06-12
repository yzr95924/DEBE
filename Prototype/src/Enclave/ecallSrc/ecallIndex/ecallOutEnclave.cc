/**
 * @file ecallSimpleIndex.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in simple index inside the enclave
 * @version 0.1
 * @date 2020-12-28
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/ecallOutEnclave.h"

/**
 * @brief Construct a new Ecall Simple Index object
 * 
 */
EcallOutEnclaveIndex::EcallOutEnclaveIndex() {
    Enclave::Logging(myName_.c_str(), "init the EcallOutEnclaveIndex.\n");
}

/**
 * @brief Destroy the Ecall Simple Index object
 * 
 */
EcallOutEnclaveIndex::~EcallOutEnclaveIndex() {
    Enclave::Logging(myName_.c_str(), "========EcallOutEnclaveIndex Info========\n");
    Enclave::Logging(myName_.c_str(), "logical chunk num: %lu\n", _logicalChunkNum);
    Enclave::Logging(myName_.c_str(), "logical data size: %lu\n", _logicalDataSize);
    Enclave::Logging(myName_.c_str(), "unique chunk num: %lu\n", _uniqueChunkNum);
    Enclave::Logging(myName_.c_str(), "unique data size: %lu\n", _uniqueDataSize);
    Enclave::Logging(myName_.c_str(), "compressed data size: %lu\n", _compressedDataSize);
    Enclave::Logging(myName_.c_str(), "================================\n");
}

/**
 * @brief process one batch
 * 
 * @param recvChunkBuf the recv chunk buffer
 * @param upOutSGX the pointer to enclave-related var 
 */
void EcallOutEnclaveIndex::ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, 
    UpOutSGX_t* upOutSGX) {
    // the in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;
    EVP_MD_CTX* mdCtx = sgxClient->_mdCtx;
    uint8_t* recvBuffer = sgxClient->_recvBuffer;
    uint8_t* sessionKey = sgxClient->_sessionKey;
    Recipe_t* inRecipe = &sgxClient->_inRecipe;

    // decrypt the received data with the session key
    cryptoObj_->SessionKeyDec(cipherCtx, recvChunkBuf->dataBuffer, 
        recvChunkBuf->header->dataSize, sessionKey, recvBuffer);

    // get the chunk num
    uint32_t chunkNum = recvChunkBuf->header->currentItemNum;

    // start to process each chunk
    string tmpChunkAddrStr;
    tmpChunkAddrStr.resize(sizeof(RecipeEntry_t), 0);
    size_t currentOffset = 0; 
    uint32_t tmpChunkSize = 0;
    string tmpHashStr;
    tmpHashStr.resize(CHUNK_HASH_SIZE, 0);
    string tmpCipherHashStr;
    tmpCipherHashStr.resize(CHUNK_HASH_SIZE, 0);
    string tmpCipherAddrStr;
    tmpCipherAddrStr.resize(sizeof(RecipeEntry_t), 0);
    bool status;

    for (size_t i = 0; i < chunkNum; i++) {
        // compute the hash over the plaintext chunk
        memcpy(&tmpChunkSize, recvBuffer + currentOffset, sizeof(tmpChunkSize));
        currentOffset += sizeof(tmpChunkSize);

        cryptoObj_->GenerateHash(mdCtx, recvBuffer + currentOffset, 
            tmpChunkSize, (uint8_t*)&tmpHashStr[0]);
        
        cryptoObj_->IndexAESCMCEnc(cipherCtx, (uint8_t*)&tmpHashStr[0], CHUNK_HASH_SIZE, 
            Enclave::indexQueryKey_, (uint8_t*)&tmpCipherHashStr[0]);

        status = this->ReadIndexStore(tmpCipherHashStr, tmpCipherAddrStr, upOutSGX);
        if (status == false) {
            // this is unique chunk
            // this chunk does not exist in the outside index
            _uniqueChunkNum++;
            _uniqueDataSize += tmpChunkSize;

            // process one unique chunk
            this->ProcessUniqueChunk((RecipeEntry_t*)&tmpChunkAddrStr[0], recvBuffer + currentOffset,
                tmpChunkSize, upOutSGX);
            
            // encrypt the chunk address
            cryptoObj_->EncryptWithKey(cipherCtx, (uint8_t*)&tmpChunkAddrStr[0], sizeof(RecipeEntry_t), 
                Enclave::indexQueryKey_, (uint8_t*)&tmpCipherAddrStr[0]);

            // update the outside index
            this->UpdateIndexStore(tmpCipherHashStr, &tmpCipherAddrStr[0], sizeof(RecipeEntry_t));
        } else {
            // this is duplicate chunk, decrypt the value
            cryptoObj_->DecryptWithKey(cipherCtx, (uint8_t*)&tmpCipherAddrStr[0], sizeof(RecipeEntry_t),
                Enclave::indexQueryKey_, (uint8_t*)&tmpChunkAddrStr[0]);
        }

        this->UpdateFileRecipe(tmpChunkAddrStr, inRecipe, upOutSGX);
        currentOffset += tmpChunkSize;

        // update the statistic
        _logicalDataSize += tmpChunkSize;
        _logicalChunkNum++;
    }

    // reset
    this->ResetCurrentSegment(sgxClient);
    memset(recvBuffer, 0, Enclave::sendChunkBatchSize_ * sizeof(Chunk_t));
    memset(recvChunkBuf->dataBuffer, 0, Enclave::sendChunkBatchSize_ * sizeof(Chunk_t));
    return ;
}

/**
 * @brief process the tailed batch when received the end of the recipe flag
 * 
 * @param upOutSGX the pointer to enclave-related var
 */
void EcallOutEnclaveIndex::ProcessTailBatch(UpOutSGX_t* upOutSGX) {
    // the in-enclave info 
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    Recipe_t* inRecipe = &sgxClient->_inRecipe;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;
    uint8_t* masterKey = sgxClient->_masterKey;

    if (inRecipe->recipeNum != 0) {
        // the out-enclave info
        Recipe_t* outRecipe = (Recipe_t*)upOutSGX->outRecipe;
        cryptoObj_->EncryptWithKey(cipherCtx, inRecipe->entryList,
            inRecipe->recipeNum * sizeof(RecipeEntry_t), masterKey, 
            outRecipe->entryList);
        outRecipe->recipeNum = inRecipe->recipeNum;
        Ocall_UpdateFileRecipe(upOutSGX->outClient);
        inRecipe->recipeNum = 0;
    }

    if (sgxClient->_inContainer.curSize != 0) {
        memcpy(upOutSGX->curContainer->body, sgxClient->_inContainer.buf,
            sgxClient->_inContainer.curSize);
        upOutSGX->curContainer->currentSize = sgxClient->_inContainer.curSize;
    }

    return ;
}