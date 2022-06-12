/**
 * @file leveldbDatabase.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implementation the database based on leveldb
 * @version 0.1
 * @date 2020-01-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef BASICDEDUP_LEVELDB_H
#define BASICDEDUP_LEVELDB_H

#include "absDatabase.h"
#include <leveldb/db.h>
#include <leveldb/cache.h>
#include "configure.h"
#include <bits/stdc++.h>


class LeveldbDatabase : public AbsDatabase {
    protected:
        /* data */
        leveldb::DB* levelDBObj_ = NULL;
        leveldb::Options options_;
    public:
        /**
         * @brief Construct a new leveldb Database object
         * 
         */
        LeveldbDatabase() {};
        /**
         * @brief Construct a new Database object
         * 
         * @param dbName the path of the db file
         */
        LeveldbDatabase(std::string dbName);
        /**
         * @brief Destroy the leveldb Database object
         * 
         */
        virtual ~LeveldbDatabase();

        /**
         * @brief open a database
         * 
         * @param dbName the db path 
         * @return true success
         * @return false fail
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



#endif // !BASICDEDUP_LEVELDB_H