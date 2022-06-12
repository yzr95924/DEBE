/**
 * @file ecallBaseline.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of baseline index
 * @version 0.1
 * @date 2021-05-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/ecallInEnclave.h"

/**
 * @brief Construct a new Ecall Baseline Index object
 * 
 * @param enclaveConfig the pointer of the enclave config
 */
EcallInEnclaveIndex::EcallInEnclaveIndex() {
    if (ENABLE_SEALING) {
        if (!this->LoadDedupIndex()) {
            Enclave::Logging(myName_.c_str(), "do not need to load the previous index.\n"); 
        } 
    }
    Enclave::Logging(myName_.c_str(), "init the EcallInEnclaveIndex.\n");

}


/**
 * @brief Destroy the Ecall Baseline Index object
 * 
 */
EcallInEnclaveIndex::~EcallInEnclaveIndex() {
    if (ENABLE_SEALING) {
        this->PersistDedupIndex();
    }
    Enclave::Logging(myName_.c_str(), "========EcallInEnclaveIndex Info========\n");
    Enclave::Logging(myName_.c_str(), "logical chunk num: %lu\n", _logicalChunkNum);
    Enclave::Logging(myName_.c_str(), "logical data size: %lu\n", _logicalDataSize);
    Enclave::Logging(myName_.c_str(), "unique chunk num: %lu\n", _uniqueChunkNum);
    Enclave::Logging(myName_.c_str(), "unique data size: %lu\n", _uniqueDataSize);
    Enclave::Logging(myName_.c_str(), "compressed data size: %lu\n", _compressedDataSize);
    Enclave::Logging(myName_.c_str(), "===================================\n");
}

/**
 * @brief persist the deduplication index to the disk
 * 
 * @return true success
 * @return false fail
 */
bool EcallInEnclaveIndex::PersistDedupIndex() {
    size_t offset = 0;
    bool persistenceStatus = false;
    uint8_t* tmpBuffer;
    size_t itemSize = 0;
    size_t maxBufferSize;

    // persist the index
    Ocall_InitWriteSealedFile(&persistenceStatus, SEALED_BASELINE_INDEX_PATH);
    if (persistenceStatus == false) {
        Ocall_SGX_Exit_Error("EcallInEnclaveIndex: cannot init the index sealed file.");
    }
    
    maxBufferSize = Enclave::sendChunkBatchSize_ * (CHUNK_HASH_SIZE + sizeof(RecipeEntry_t));
    tmpBuffer = (uint8_t*) malloc(maxBufferSize);
    itemSize = insideIndexObj_.size();

    // persist the item number
    Enclave::WriteBufferToFile((uint8_t*)&itemSize, sizeof(itemSize), SEALED_BASELINE_INDEX_PATH);

    // start to persist the index item 
    for (auto it = insideIndexObj_.begin(); it != insideIndexObj_.end(); it++) {
        memcpy(tmpBuffer + offset, it->first.c_str(), CHUNK_HASH_SIZE);
        offset += CHUNK_HASH_SIZE;
        memcpy(tmpBuffer + offset, it->second.c_str(), sizeof(RecipeEntry_t));
        offset += sizeof(RecipeEntry_t);
        if (offset == maxBufferSize) {
            // the buffer is full, write to the file
            Enclave::WriteBufferToFile(tmpBuffer, offset, SEALED_BASELINE_INDEX_PATH);
            offset = 0;
        }
    }

    if (offset != 0) {
        // handle the tail data
        Enclave::WriteBufferToFile(tmpBuffer, offset, SEALED_BASELINE_INDEX_PATH);
        offset = 0;
    }
    Ocall_CloseWriteSealedFile(SEALED_BASELINE_INDEX_PATH);
    free(tmpBuffer);
    return true;
}

/**
 * @brief read the hook index from sealed data
 * 
 * @return true success
 * @return false fail
 */
bool EcallInEnclaveIndex::LoadDedupIndex() {
    size_t itemNum;
    string keyStr;
    keyStr.resize(CHUNK_HASH_SIZE, 0);
    string valueStr;
    valueStr.resize(sizeof(RecipeEntry_t), 0); 
    size_t offset = 0;
    size_t maxBufferSize = 0;
    uint8_t* tmpBuffer;

    size_t sealedDataSize; 
    Ocall_InitReadSealedFile(&sealedDataSize, SEALED_BASELINE_INDEX_PATH); 
    if (sealedDataSize == 0) {
        return false;
    }

    // read the item number;
    Enclave::ReadFileToBuffer((uint8_t*)&itemNum, sizeof(itemNum), SEALED_BASELINE_INDEX_PATH); 

    maxBufferSize = Enclave::sendChunkBatchSize_ * (CHUNK_HASH_SIZE + sizeof(RecipeEntry_t));
    tmpBuffer = (uint8_t*) malloc(maxBufferSize);

    size_t expectReadBatchNum = (itemNum / Enclave::sendChunkBatchSize_);
    for (size_t i = 0; i < expectReadBatchNum; i++) {
        Enclave::ReadFileToBuffer(tmpBuffer, maxBufferSize, SEALED_BASELINE_INDEX_PATH);
        for (size_t item = 0; item < Enclave::sendChunkBatchSize_; 
            item++) {
            memcpy(&keyStr[0], tmpBuffer + offset, CHUNK_HASH_SIZE);
            offset += CHUNK_HASH_SIZE;
            memcpy(&valueStr[0], tmpBuffer + offset, sizeof(RecipeEntry_t));
            offset += sizeof(RecipeEntry_t);

            // update the index
            insideIndexObj_.insert({keyStr, valueStr});
        } 
        offset = 0;
    }

    size_t remainItemNum = itemNum - insideIndexObj_.size();
    if (remainItemNum != 0) {
        Enclave::ReadFileToBuffer(tmpBuffer, maxBufferSize, SEALED_BASELINE_INDEX_PATH);
        for (size_t i = 0; i < remainItemNum; i++) {
            memcpy(&keyStr[0], tmpBuffer + offset, CHUNK_HASH_SIZE);
            offset += CHUNK_HASH_SIZE;
            memcpy(&valueStr[0], tmpBuffer + offset, sizeof(RecipeEntry_t));
            offset += sizeof(RecipeEntry_t);  

            // update the index
            insideIndexObj_.insert({keyStr, valueStr});
        }
        offset = 0;
    }

    Ocall_CloseReadSealedFile(SEALED_BASELINE_INDEX_PATH);
    free(tmpBuffer);

    // check the index size consistency 
    if (insideIndexObj_.size() != itemNum) {
        Ocall_SGX_Exit_Error("EcallInEnclaveIndex: load the index error.");
    }
    return true;
}


/**
 * @brief process one batch
 * 
 * @param buffer the input buffer
 * @param payloadSize the payload size
 * @param upOutSGX the pointer to enclave-related var
 */
void EcallInEnclaveIndex::ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf,
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
    string tmpChunkAddressStr;
    tmpChunkAddressStr.resize(sizeof(RecipeEntry_t), 0);
    size_t currentOffset = 0;
    uint32_t tmpChunkSize = 0;
    string tmpHashStr;
    tmpHashStr.resize(CHUNK_HASH_SIZE, 0);

    for (size_t i = 0; i < chunkNum; i++) {
        // compute the hash over the plaintext chunk
        memcpy(&tmpChunkSize, recvBuffer + currentOffset, sizeof(uint32_t));
        currentOffset += sizeof(uint32_t);

        cryptoObj_->GenerateHash(mdCtx, recvBuffer + currentOffset, 
            tmpChunkSize, (uint8_t*)&tmpHashStr[0]);
        
        if (insideIndexObj_.count(tmpHashStr) != 0) {
            // it is duplicate chunk
            tmpChunkAddressStr.assign(insideIndexObj_[tmpHashStr]);
        } else {
            // it is unique chunk
            _uniqueChunkNum++;
            _uniqueDataSize += tmpChunkSize;

            // process one unique chunk
            this->ProcessUniqueChunk((RecipeEntry_t*)&tmpChunkAddressStr[0], 
                recvBuffer + currentOffset, tmpChunkSize, upOutSGX);

            // update the index 
            insideIndexObj_[tmpHashStr] = tmpChunkAddressStr;
        }

        this->UpdateFileRecipe(tmpChunkAddressStr, inRecipe, upOutSGX);
        currentOffset += tmpChunkSize;

        // update the statistic
        _logicalDataSize += tmpChunkSize;
        _logicalChunkNum++;
    }

    return ;
}

/**
 * @brief process the tailed batch when received the end of the recipe flag
 * 
 * @param upOutSGX the pointer to enclave-related var
 */
void EcallInEnclaveIndex::ProcessTailBatch(UpOutSGX_t* upOutSGX) {
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