/**
 * @file ecallStorage.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of storage core inside the enclave
 * @version 0.1
 * @date 2020-12-16
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef ECALL_STORAGE_H
#define ECALL_STORAGE_H

#include "commonEnclave.h"

class EcallStorageCore {
    private:
        string myName_ = "StorageCore"; 

        // written data size
        uint64_t writtenDataSize_ = 0;
        uint64_t writtenChunkNum_ = 0;

    public:
        /**
         * @brief Construct a new Ecall Storage Core object
         * 
         */
        EcallStorageCore();

        /**
         * @brief Destroy the Ecall Storage Core object
         * 
         */
        ~EcallStorageCore();

        /**
         * @brief save the chunk to the storage serve
         * 
         * @param chunkData the chunk data buffer
         * @param chunkSize the chunk size
         * @param chunkAddr the chunk address (return)
         * @param upOutSGX the pointer to outside SGX buffer
         */
        void SaveChunk(char* chunkData, uint32_t chunkSize,
            RecipeEntry_t* chunkAddr, UpOutSGX_t* upOutSGX);
};

#endif