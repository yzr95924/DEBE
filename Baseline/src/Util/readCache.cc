/**
 * @file readCache.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in readCache.h
 * @version 0.1
 * @date 2020-02-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/readCache.h"
#include <boost/thread/mutex.hpp>

/**
 * @brief Construct a new Read Cache object
 * 
 */
ReadCache::ReadCache() {
    this->readCache_ = new lru11::Cache<string, uint32_t>(config.GetReadCacheSize(), 0);
    cacheSize_ = config.GetReadCacheSize();
    containerPool_ = (uint8_t**) malloc(cacheSize_ * sizeof(uint8_t*));
    for (size_t i = 0; i < cacheSize_; i++) {
        containerPool_[i] = (uint8_t*) malloc(MAX_CONTAINER_SIZE * sizeof(uint8_t));
    }
    currentIndex_ = 0;
}

/**
 * @brief Destroy the Read Cache object
 * 
 */
ReadCache::~ReadCache() {
    for (size_t i = 0; i < cacheSize_; i++) {
        free(containerPool_[i]);
    }
    free(containerPool_);
    delete readCache_;
}

/**
 * @brief insert the data to the cache
 * 
 * @param name key - container ID
 * @param data data
 * @param length the length of the container section
 */
void ReadCache::InsertToCache(string& name, uint8_t* data, uint32_t length) {
    if (readCache_->size() + 1 > cacheSize_) {
        // evict a item
        uint32_t replaceIndex = readCache_->pruneValue();
        // fprintf(stdout, "Cache evict: %u\n", replaceIndex);
        memcpy(containerPool_[replaceIndex], data, length);
        readCache_->insert(name, replaceIndex);
    } else {
        // directly using current index
        // fprintf(stdout, "Cache use: %lu\n", currentIndex_);
        memcpy(containerPool_[currentIndex_], data, length);
        readCache_->insert(name, currentIndex_);
        currentIndex_++;
    }
}

/**
 * @brief check whether this item exists in the cache
 * 
 * @param name 
 * @return true 
 * @return false 
 */
bool ReadCache::ExistsInCache(string& name) {
    bool flag = false;
    flag = this->readCache_->contains(name);
    return flag;
}

/**
 * @brief read the data from the cache
 * 
 * @param data key
 * @return string the data
 */
uint8_t* ReadCache::ReadFromCache(string& name) {
    uint32_t index = this->readCache_->get(name);
    return containerPool_[index];
}