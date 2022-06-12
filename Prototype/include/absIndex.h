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
#include "chunkStructure.h"
#include "cryptoPrimitive.h"
#include "messageQueue.h"
#include "storageCore.h"

using namespace std;

class AbsIndex {
    protected:
        // the pointer to the abs database
        AbsDatabase* indexStore_;

        // for storageCore
        StorageCore* storageCoreObj_;

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

    public:
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
        virtual ~AbsIndex() ;

        /**
         * @brief process the tail segment
         * 
         * @param upOutSGX the structure to store the enclave related variable
         */
        virtual void ProcessTailBatch(UpOutSGX_t* upOutSGX) = 0;

        /**
         * @brief process one batch 
         * 
         * @param recvChunkBuf the recv chunk buffer
         * @param upOutSGX the structure to store the enclave related variable
         */
        virtual void ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, 
            UpOutSGX_t* upOutSGX) = 0;

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