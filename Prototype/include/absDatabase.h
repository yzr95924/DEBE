/**
 * @file absDatabase.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2020-01-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef BASICDEDUP_ABS_DATABASE_H
#define BASICDEDUP_ABS_DATABASE_H

#include "configure.h"

using namespace std;

class AbsDatabase {
    protected:
        // the name of the database
        std::string dbName_;
    public:
        /**
         * @brief Construct a new Abs Database object
         * 
         */
        AbsDatabase(); 

        /**
         * @brief Destroy the Abs Database object
         * 
         */
        virtual ~AbsDatabase() {}; 

        /**
         * @brief open a database
         * 
         * @param dbName the db path 
         * @return true success
         * @return false fail
         */
        virtual bool OpenDB(std::string dbName) = 0;


        /**
         * @brief execute query over database
         * 
         * @param key key
         * @param value value
         * @return true success
         * @return false fail
         */
        virtual bool Query(const std::string& key, std::string& value) = 0;

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key key
         * @param value value
         * @return true success
         * @return false fail
         */
        virtual bool Insert(const std::string& key, const std::string& value) = 0;


        /**
         * @brief insert the (key, value) pair
         * 
         * @param key 
         * @param buffer 
         * @param bufferSize 
         * @return true 
         * @return false 
         */
        virtual bool InsertBuffer(const std::string& key, const char* buffer, size_t bufferSize) = 0;

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
        virtual bool InsertBothBuffer(const char* key, size_t keySize, const char* buffer,
            size_t bufferSize) = 0;

        /**
         * @brief query the (key, value) pair
         * 
         * @param key 
         * @param keySize 
         * @param value 
         * @return true 
         * @return false 
         */
        virtual bool QueryBuffer(const char* key, size_t keySize, std::string& value) = 0;


};


#endif // !BASICDEDUP_ABS_DATABASE_H

