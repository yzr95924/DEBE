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
    string myName_ = "Configure";
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
    
    // restore setting
    uint64_t readCacheSize_;
    
    // for storage ip
    string storageServerIp_;
    int storageServerPort_;

    // client id
    uint32_t clientID_;
    uint64_t sendChunkBatchSize_ = 0;
    uint64_t sendRecipeBatchSize_ = 0;

    // for local secret 
    string localSecret_;

    // for key server
    string keyServerIp_;
    int keyServerPort_;

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
    
    uint64_t GetChunkingType() {
        return chunkingType_;
    }

    uint64_t GetMaxChunkSize() {
        return maxChunkSize_;
    }

    uint64_t GetMinChunkSize() {
        return minChunkSize_;
    }

    uint64_t GetAvgChunkSize() {
        return avgChunkSize_;
    }

    uint64_t GetSlidingWinSize() {
        return slidingWinSize_;
    }

    uint64_t GetReadSize() {
        return readSize_;
    }
    
    string GetRecipeRootPath() {
        return recipeRootPath_;
    }

    string GetRecipeSuffix() {
        return recipeSuffix_;
    }

    string GetContainerRootPath() {
        return containerRootPath_;
    }

    string GetContainerSuffix() {
        return containerSuffix_;
    }

    string GetLocalSecret() {
        return localSecret_;
    }

    string GetFp2ChunkDBName() {
        return fp2ChunkDBName_;
    }

    uint64_t GetReadCacheSize() {
        return readCacheSize_;
    }

    string GetStorageServerIP() {
        return storageServerIp_;
    }
    
    int GetStoragePort() {
        return storageServerPort_;
    }

    uint32_t GetClientID() {
        return clientID_;
    }

    inline uint64_t GetSendChunkBatchSize() {
        // return sendChunkBatchSize_;
        return 256;
    }

    inline uint64_t GetSendRecipeBatchSize() {
        // return sendRecipeBatchSize_;
        if (chunkingType_ == 1) {
            return 512;
        } else {
            return 1024;
        }
    }
    
    // key server config
    string GetKeyServerIP() {
        return keyServerIp_;
    }
    int GetKeyServerPort() {
        return keyServerPort_;
    }

};

#endif //BASICDEDUP_CONFIGURE_h