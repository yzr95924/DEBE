/**
 * @file absIndex.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in abs index 
 * @version 0.1
 * @date 2020-11-30
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/absIndex.h"

/**
 * @brief Construct a new Abs Index object
 * 
 * @param indexStore the pointer to the index store 
 */
AbsIndex::AbsIndex(AbsDatabase* indexStore) {
    indexStore_ = indexStore;
    cryptoObj_ = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);
    sendChunkBatchSize_ = config.GetSendChunkBatchSize();
    sendRecipeBatchSize_ = config.GetSendRecipeBatchSize();

    if (tool::FileExist(persistentFileName_)) {
        // the stat file exists
        ifstream previousStatFile;
        previousStatFile.open(persistentFileName_, ios_base::binary | ios_base::in);
        if (!previousStatFile.is_open()) {
            tool::Logging(myName_.c_str(), "cannot open the stat file.\n");
            exit(EXIT_FAILURE);
        } else {
            previousStatFile.read((char*)&_logicalDataSize, sizeof(uint64_t));
            previousStatFile.read((char*)&_logicalChunkNum, sizeof(uint64_t));
            previousStatFile.read((char*)&_uniqueDataSize, sizeof(uint64_t));
            previousStatFile.read((char*)&_uniqueChunkNum, sizeof(uint64_t));
            previousStatFile.read((char*)&_compressedDataSize, sizeof(uint64_t));
        }
    }
}

/**
 * @brief Destroy the Abs Index object
 * 
 */
AbsIndex::~AbsIndex() {
    ofstream previousStatFile;
    previousStatFile.open(persistentFileName_, ios_base::trunc);
    if (!previousStatFile.is_open()) {
        tool::Logging(myName_.c_str(), "cannot open the stat file.\n");
    } else {
        previousStatFile.write((char*)&_logicalDataSize, sizeof(uint64_t));
        previousStatFile.write((char*)&_logicalChunkNum, sizeof(uint64_t));
        previousStatFile.write((char*)&_uniqueDataSize, sizeof(uint64_t));
        previousStatFile.write((char*)&_uniqueChunkNum, sizeof(uint64_t));
        previousStatFile.write((char*)&_compressedDataSize, sizeof(uint64_t));
    }
    delete cryptoObj_;
}

/**
 * @brief read the information from the index store
 * 
 * @param key key 
 * @param value value
 * @return true success
 * @return false fail
 */
bool AbsIndex::ReadIndexStore(const string& key, string& value) {
    return indexStore_->Query(key, value);
}


/**
 * @brief update the index store
 * 
 * @param key key
 * @param value value 
 * @return true success 
 * @return false fail
 */
bool AbsIndex::UpdateIndexStore(const string& key, const string& value) {
    return indexStore_->Insert(key, value);
}


/**
 * @brief update the index store
 * 
 * @param key key
 * @param buffer the pointer to the buffer
 * @param bufferSize buffer size
 * @return true success
 * @return false fail
 */
bool AbsIndex::UpdateIndexStore(const string& key, const char* buffer, size_t bufferSize) {
    return indexStore_->InsertBuffer(key, buffer, bufferSize);
}

/**
 * @brief process one unique chunk
 * 
 * @param chunkAddr the chunk address
 * @param chunkBuffer the chunk buffer
 * @param chunkSize the chunk size
 * @param curClient the current client var
 */
void AbsIndex::ProcessUniqueChunk(RecipeEntry_t* chunkAddr, uint8_t* chunkBuffer,
    uint32_t chunkSize, ClientVar* curClient) {
    uint8_t tmpCompressedChunk[MAX_CHUNK_SIZE];
    int tmpCompressedChunkSize = 0;
    tmpCompressedChunkSize = LZ4_compress_fast((char*)(chunkBuffer), (char*)tmpCompressedChunk,
        chunkSize, chunkSize, 3);
    
    if (tmpCompressedChunkSize > 0) {
        // it can be compressed
        _compressedDataSize += tmpCompressedChunkSize;
    } else {
        // it cannot be compressed
        _compressedDataSize += chunkSize;
        tmpCompressedChunkSize = chunkSize;

        memcpy(tmpCompressedChunk, chunkBuffer, tmpCompressedChunkSize);
    }

    // finish the compression, assign this a container
    storageCoreObj_->SaveChunk((char*)tmpCompressedChunk, tmpCompressedChunkSize, 
        chunkAddr, curClient);
    return ;
}

/**
 * @brief update the file recipe
 * 
 * @param chunkAddrStr the chunk address string
 * @param inRecipe the recipe buffer
 * @param curClient the current client var
 */
void AbsIndex::UpdateFileRecipe(string &chunkAddrStr, Recipe_t* inRecipe, 
    ClientVar* curClient) {
    memcpy(inRecipe->entryList + inRecipe->recipeNum * sizeof(RecipeEntry_t), 
        chunkAddrStr.c_str(), sizeof(RecipeEntry_t));
    inRecipe->recipeNum++;

    if ((inRecipe->recipeNum % sendRecipeBatchSize_) == 0) {
        storageCoreObj_->UpdateRecipeToFile(inRecipe->entryList, 
            inRecipe->recipeNum, curClient->_recipeWriteHandler);
        inRecipe->recipeNum = 0; 
    }
    return ;
}