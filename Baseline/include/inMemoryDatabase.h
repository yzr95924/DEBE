/**
 * @file inMemoryIndex.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement a in-memory index
 * @version 0.1
 * @date 2021-05-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef IN_MEMORY_INDEX_H
#define IN_MEMORY_INDEX_H

#include "absDatabase.h"
#include "configure.h"

class InMemoryDatabase : public AbsDatabase {
    protected:
        /*data*/
        unordered_map<string, string> indexObj_;
    public:
        /**
         * @brief Construct a new In Memory Database object
         * 
         */
        InMemoryDatabase() {};

        /**
         * @brief Construct a new In Memory Database object
         * 
         * @param dbName the path of the db file
         */
        InMemoryDatabase(std::string dbName);

        /**
         * @brief Destroy the In Memory Database object
         * 
         */
        virtual ~InMemoryDatabase();

        /**
         * @brief open a database
         * 
         * @param dbName the db path
         * @return true success
         * @return false fails
         */
        bool OpenDB(std::string dbName);

        /**
         * @brief execute query over database
         * 
         * @param key key
         * @param value value
         * @return true success
         * @return false fail
         */
        bool Query(const std::string& key, std::string& value);

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key key
         * @param value value
         * @return true success
         * @return false fail
         */
        bool Insert(const std::string& key, const std::string& value);


        /**
         * @brief insert the (key, value) pair
         * 
         * @param key 
         * @param buffer 
         * @param bufferSize 
         * @return true 
         * @return false 
         */
        bool InsertBuffer(const std::string& key, const char* buffer, size_t bufferSize);

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key 
         * @param keySize 
         * @param buffer 
         * @param bufferSize 
         * @return true 
         * @return false 
         */
        bool InsertBothBuffer(const char* key, size_t keySize, const char* buffer,
            size_t bufferSize);

        /**
         * @brief query the (key, value) pair
         * 
         * @param key 
         * @param keySize 
         * @param value 
         * @return true 
         * @return false 
         */
        bool QueryBuffer(const char* key, size_t keySize, std::string& value);

};

#endif