/**
 * @file plainIndex.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement plain index
 * @version 0.1
 * @date 2021-05-14
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/plainIndex.h"

/**
 * @brief Construct a new Plain Index object
 * 
 * @param indexStore the reference to the index store
 */
PlainIndex::PlainIndex(AbsDatabase* indexStore) : AbsIndex (indexStore) {
    tool::Logging(myName_.c_str(), "init the PlainIndex.\n");
}

/**
 * @brief Destroy the Plain Index object destore the plain index
 * 
 */
PlainIndex::~PlainIndex() {
    fprintf(stderr, "========PlainIndex Info========\n");
    fprintf(stderr, "recv data size: %lu\n", totalRecvDataSize_);
    fprintf(stderr, "recv batch num: %lu\n", totalBatchNum_);
    fprintf(stderr, "logical chunk num: %lu\n", _logicalChunkNum);
    fprintf(stderr, "logical data size: %lu\n", _logicalDataSize);
    fprintf(stderr, "unique chunk num: %lu\n", _uniqueChunkNum);
    fprintf(stderr, "unique data size: %lu\n", _uniqueDataSize);
    fprintf(stderr, "compressed data size: %lu\n", _compressedDataSize);
    fprintf(stderr, "===============================\n");
}

/**
 * @brief process one batch 
 * 
 * @param recvChunkBuf the recv chunk buffer
 * @param curClient the current client var
 */
void PlainIndex::ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, ClientVar* curClient) {
    // update statistic
    totalRecvDataSize_ += recvChunkBuf->header->dataSize;
    totalBatchNum_++;

    // the client info
    EVP_MD_CTX* mdCtx = curClient->_mdCtx;
    Recipe_t* inRecipe = &curClient->_inRecipe;

    // get the chunk num
    uint32_t chunkNum = recvChunkBuf->header->currentItemNum;

    // start to process each chunk
    string tmpChunkAddStr;
    tmpChunkAddStr.resize(sizeof(RecipeEntry_t), 0);
    size_t currentOffset = 0;
    uint32_t tmpChunkSize = 0;
    string tmpHashStr;
    tmpHashStr.resize(CHUNK_HASH_SIZE, 0);
    bool status;

    for (size_t i = 0; i < chunkNum; i++) {
        // compute the hash over the ciphertext chunk
        memcpy(&tmpChunkSize, recvChunkBuf->dataBuffer + currentOffset, sizeof(tmpChunkSize));
        currentOffset += sizeof(tmpChunkSize);

        cryptoObj_->GenerateHash(mdCtx, recvChunkBuf->dataBuffer + currentOffset, 
            tmpChunkSize, (uint8_t*)&tmpHashStr[0]);
        status = this->ReadIndexStore(tmpHashStr, tmpChunkAddStr);
        if (status == false) {
            // this is unique chunk
            _uniqueChunkNum++;
            _uniqueDataSize += tmpChunkSize;

            // process one unique chunk
            this->ProcessUniqueChunk((RecipeEntry_t*)&tmpChunkAddStr[0], recvChunkBuf->dataBuffer + currentOffset, 
                tmpChunkSize, curClient);
            
            // update the index
            this->UpdateIndexStore(tmpHashStr, tmpChunkAddStr);
        }

        this->UpdateFileRecipe(tmpChunkAddStr, inRecipe, curClient);
        currentOffset += tmpChunkSize;

        // update the statistic
        _logicalDataSize += tmpChunkSize;
        _logicalChunkNum++;
    }

    // reset
    memset(recvChunkBuf->dataBuffer, 0, sendChunkBatchSize_ * sizeof(Chunk_t));
    return ;
}

/**
 * @brief process the tail segment
 * 
 * @param curClient the current client var
 */
void PlainIndex::ProcessTailBatch(ClientVar* curClient) {
    Recipe_t* inRecipe = &curClient->_inRecipe;

    if (inRecipe->recipeNum != 0) {
        storageCoreObj_->UpdateRecipeToFile(inRecipe->entryList, 
            inRecipe->recipeNum, curClient->_recipeWriteHandler);
        inRecipe->recipeNum = 0;
    }
    return ;
}