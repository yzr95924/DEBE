/**
 * @file absIndex.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of absIndex 
 * @version 0.1
 * @date 2020-11-30
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef ABS_INDEX_H 
#define ABS_INDEX_H

#include "configure.h"
#include "absDatabase.h"
#include "cryptoPrimitive.h"
#include "storageCore.h"
#include "clientVar.h"
#include <lz4.h>

extern Configure config;


class AbsIndex {
    protected:
        // the pointer to the abs database
        string myName_ = "AbsIndex";
        AbsDatabase* indexStore_;
        string persistentFileName_ = "previous-stat";

        // for config
        uint64_t sendChunkBatchSize_ = 0;
        uint64_t sendRecipeBatchSize_ = 0;

        // for storageCore
        StorageCore* storageCoreObj_;

        // for crypto
        CryptoPrimitive* cryptoObj_;

        // for statistic
        uint64_t totalRecvDataSize_ = 0;
        uint64_t totalBatchNum_ = 0; 

        /**
         * @brief read the information from the 
         * 
         * @param key key 
         * @param value value
         * @return true success
         * @return false fail
         */
        bool ReadIndexStore(const string& key, string& value);

        /**
         * @brief update the index store
         * 
         * @param key key
         * @param value value 
         * @return true success 
         * @return false fail
         */
        bool UpdateIndexStore(const string& key, const string& value);

        /**
         * @brief update the index store
         * 
         * @param key key
         * @param buffer the pointer to the buffer
         * @param bufferSize buffer size
         * @return true success
         * @return false fail
         */
        bool UpdateIndexStore(const string& key, const char* buffer, size_t bufferSize);

        /**
         * @brief process one unique chunk
         * 
         * @param chunkAddr the chunk address
         * @param chunkBuffer the chunk buffer
         * @param chunkSize the chunk size
         * @param curClient the current client var
         */
        void ProcessUniqueChunk(RecipeEntry_t* chunkAddr, uint8_t* chunkBuffer,
            uint32_t chunkSize, ClientVar* curClient);

        /**
         * @brief update the file recipe
         * 
         * @param chunkAddrStr the chunk address string
         * @param inRecipe the recipe buffer
         * @param curClient the current client var
         */
        void UpdateFileRecipe(string &chunkAddrStr, Recipe_t* inRecipe, 
            ClientVar* curClient);

    public:
        // for statistic
        uint64_t _logicalChunkNum = 0;
        uint64_t _logicalDataSize = 0;
        uint64_t _uniqueChunkNum = 0;
        uint64_t _uniqueDataSize = 0;
        uint64_t _compressedDataSize = 0;

        /**
         * @brief Construct a new Abs Index object
         * 
         * @param indexStore a pointer to the index store
         */
        AbsIndex(AbsDatabase* indexStore);        

        /**
         * @brief Destroy the Abs Index object
         * 
         */
        virtual ~AbsIndex();

        /**
         * @brief process one batch 
         * 
         * @param recvChunkBuf the recv chunk buffer
         * @param curClient the current client var
         */
        virtual void ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, ClientVar* curClient) = 0;

        /**
         * @brief process the tail segment
         * 
         * @param curClient the current client var
         */
        virtual void ProcessTailBatch(ClientVar* curClient) = 0;

        /**
         * @brief Set the Storage Core Obj object
         * 
         * @param storageCoreObj the pointer to the storageCoreObj
         */
        void SetStorageCoreObj(StorageCore* storageCoreObj) {
            storageCoreObj_ = storageCoreObj;
            return ;
        }
};


#endif // !1