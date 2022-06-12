/**
 * @file storageCore.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces defined in the storage core.
 * @version 0.1
 * @date 2019-12-27
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include "../../include/storageCore.h"

extern Configure config;

/**
 * @brief Construct a new Storage Core object
 * 
 */
StorageCore::StorageCore() {
    tool::Logging(myName_.c_str(), "Init the StorageCore\n");
}

/**
 * @brief Destroy the Storage Core:: Storage Core object
 * 
 */
StorageCore::~StorageCore() {
    fprintf(stderr, "========StorageCore Info========\n");
    fprintf(stderr, "write the data size: %lu\n", writtenDataSize_);
    fprintf(stderr, "write chunk num: %lu\n", writtenChunkNum_);
    fprintf(stderr, "================================\n");

}

/**
 * @brief write the data to a container according to the given metadata
 * 
 * @param key chunk metadata
 * @param data content
 * @param curContainer the pointer to the current container
 * @param curClient the ptr to the current client
 */
void StorageCore::WriteContainer(RecipeEntry_t* key, char* data,
    Container_t* curContainer, ClientVar* curClient) {
    uint32_t chunkSize = key->length;
    uint32_t saveOffset = curContainer->currentSize;
    uint32_t writeOffset = saveOffset;

    if (chunkSize + saveOffset < MAX_CONTAINER_SIZE) {
        // current container cannot store this chunk
        // copy data to this container
        memcpy(curContainer->body + writeOffset, data, chunkSize);
        memcpy(key->containerName, curContainer->containerID, CONTAINER_ID_LENGTH);
    } else {
        // current container cannot store this chunk, write this container to the outside buffer
        // create a new container for this new chunk
        Ocall_WriteContainer(curClient);
        // reset this container during the ocall

        saveOffset = 0;
        writeOffset = saveOffset;
        memcpy(curContainer->body + writeOffset, data, chunkSize);
        memcpy(key->containerName, curContainer->containerID, CONTAINER_ID_LENGTH);
    }
    // update the metadata of the container
    curContainer->currentSize += chunkSize;

    key->offset = saveOffset;
    return ;
}

/**
 * @brief save the chunk to the storage server
 * 
 * @param chunkData the chunk data buffer
 * @param chunkSize the chunk size
 * @param chunkAddr the chunk address (return)
 * @param curClient the prt to current client
 */
void StorageCore::SaveChunk(char* chunkData, uint32_t chunkSize, RecipeEntry_t* chunkAddr, 
    ClientVar* curClient) {
    // assign a chunk length
    chunkAddr->length = chunkSize;
    Container_t* curContainer = &curClient->_curContainer;
        
    // write to the container
    this->WriteContainer(chunkAddr, chunkData, curContainer, curClient);
    writtenDataSize_ += chunkSize;
    writtenChunkNum_++;

    return ;
}

/**
 * @brief update the file recipe to the disk
 * 
 * @param recipeBuffer the pointer to the recipe buffer
 * @param recipeEntryNum the number of recipe entries
 * @param fileRecipeHandler the recipe file handler
 */
void StorageCore::UpdateRecipeToFile(const uint8_t* recipeBuffer, size_t recipeEntryNum, 
    ofstream& fileRecipeHandler) {
    if (!fileRecipeHandler.is_open()) {
        tool::Logging(myName_.c_str(), "recipe file does not open.\n");
        exit(EXIT_FAILURE);
    }
    size_t recipeBufferSize = recipeEntryNum * sizeof(RecipeEntry_t);
    fileRecipeHandler.write((char*)recipeBuffer, recipeBufferSize);
    return ;
}

/**
 * @brief finalize the file recipe
 * 
 * @param recipeHead the recipe header
 * @param fileRecipeHandler the recipe file handler
 */
void StorageCore::FinalizeRecipe(FileRecipeHead_t* recipeHead, 
    ofstream& fileRecipeHandler) {
    if (!fileRecipeHandler.is_open()) {
        tool::Logging(myName_.c_str(), "recipe file does not open.\n");
        exit(EXIT_FAILURE);
    }
    fileRecipeHandler.seekp(0, ios_base::beg);
    fileRecipeHandler.write((const char*)recipeHead, sizeof(FileRecipeHead_t));

    fileRecipeHandler.close();    
    return ; 
}