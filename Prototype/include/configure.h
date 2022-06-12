/**
 * @file configure.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the necessary variables in deduplication
 * @version 0.1
 * @date 2019-12-19
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef BASICDEDUP_CONFIGURE_h
#define BASICDEDUP_CONFIGURE_h

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "define.h"
#include "constVar.h"
using namespace std;

class Configure {
private:
    // chunking setting
    uint64_t chunkingType_; // varSize \ fixedSize \ simple
    uint64_t maxChunkSize_;
    uint64_t avgChunkSize_;
    uint64_t minChunkSize_;
    uint64_t slidingWinSize_;
    uint64_t readSize_; //128MB per time 
    
    // deduplication setting 
    string recipeRootPath_;
    string recipeSuffix_ = "-recipe";
    string containerRootPath_;
    string containerSuffix_ = "-container";
    string fp2ChunkDBName_;
    uint64_t topKParam_;
    
    // restore setting
    uint64_t readCacheSize_;
    
    // for storage ip
    string storageServerIp_;
    int storageServerPort_;

    // client id
    uint32_t clientID_;
    uint64_t sendChunkBatchSize_ = 0;
    uint64_t sendRecipeBatchSize_ = 0;

    // for RA
    string spid_;
    uint16_t quoteType_;
    uint32_t iasServerType_;
    string iasPrimaryKey_;
    string iasSecKey_;
    uint16_t iasVersion_;

    // for local secret 
    string localSecret_;

    /**
     * @brief read the configure file
     * 
     * @param path the configure file name
     */
    void ReadConf(std::string path);

public:

    /**
     * @brief Construct a new Configure object
     * 
     * @param path the input configure file path
     */
    Configure(std::string path);

    /**
     * @brief Destroy the Configure object
     * 
     */
    ~Configure();
    
    inline uint64_t GetChunkingType() {
        return chunkingType_;
    }

    inline uint64_t GetMaxChunkSize() {
        return maxChunkSize_;
    }

    inline uint64_t GetMinChunkSize() {
        return minChunkSize_;
    }

    inline uint64_t GetAvgChunkSize() {
        return avgChunkSize_;
    }

    inline uint64_t GetSlidingWinSize() {
        return slidingWinSize_;
    }

    inline uint64_t GetReadSize() {
        return readSize_;
    }
    
    inline string GetRecipeRootPath() {
        return recipeRootPath_;
    }

    inline string GetRecipeSuffix() {
        return recipeSuffix_;
    }

    inline string GetContainerRootPath() {
        return containerRootPath_;
    }

    inline string GetContainerSuffix() {
        return containerSuffix_;
    }

    inline string GetLocalSecret() {
        return localSecret_;
    }

    inline string GetFp2ChunkDBName() {
        return fp2ChunkDBName_;
    }

    inline uint64_t GetReadCacheSize() {
#if (MULTI_CLIENT == 1)
        return 1;
#else
        return readCacheSize_;
#endif
    }

    inline string GetStorageServerIP() {
        return storageServerIp_;
    }
    
    inline int GetStoragePort() {
        return storageServerPort_;
    }

    inline uint32_t GetClientID() {
        return clientID_;
    }

    inline uint64_t GetSendChunkBatchSize() {
        return sendChunkBatchSize_;
    }

    inline uint64_t GetSendRecipeBatchSize() {
        return sendRecipeBatchSize_;
    }

    inline string GetSPID() {
        return spid_;
    }

    inline uint16_t GetQuoteType() {
        return quoteType_;
    }

    inline uint32_t GetIASServerType() {
        return iasServerType_;
    }

    inline string GetIASPrimaryKey() {
        return iasPrimaryKey_; 
    }

    inline string GetIASSecKey() {
        return iasSecKey_;
    }

    inline uint16_t GetIASVersion() {
        return iasVersion_;
    }

    inline uint64_t GetTopKParam() {
        return (topKParam_ * 1024);
    }
};

#endif