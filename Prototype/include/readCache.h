/**
 * @file readCache.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of read container cache 
 * @version 0.1
 * @date 2020-02-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef BASICDEDUP_READCACHE_H
#define BASICDEDUP_READCACHE_H

#include "configure.h"
#include "lruCache.h"

using namespace std;

extern Configure config;

class ReadCache {
    private:
        // the cache pointer
        // boost::compute::detail::lru_cache<string, string>* readCache_;
        // <container-ID, index of container pool>
        lru11::Cache<string, uint32_t>* readCache_;

        // container cache space pointer
        uint8_t** containerPool_;

        // container cache size
        uint64_t cacheSize_ = 0;

        size_t currentIndex_ = 0;
    public:
        /**
         * @brief Construct a new Read Cache object
         * 
         */
        ReadCache();

        /**
         * @brief Destroy the Read Cache object
         * 
         */
        ~ReadCache();

        /**
         * @brief insert the data to the cache
         * 
         * @param name key
         * @param data data
         * @param length the length of the container section
         */
        void InsertToCache(string& name, uint8_t* data, uint32_t length);


        /**
         * @brief check whether this item exists in the cache
         * 
         * @param name 
         * @return true 
         * @return false 
         */
        bool ExistsInCache(string& name);


        /**
         * @brief read container from cache
         * 
         * @param name id of the container
         * @return uint8_t* container data
         */
        uint8_t* ReadFromCache(string& name);

};

#endif // !BASICDEDUP_READCACHE_H




