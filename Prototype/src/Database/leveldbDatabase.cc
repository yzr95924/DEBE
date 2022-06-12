/**
 * @file leveldbDatabase.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in leveldb database
 * @version 0.1
 * @date 2020-01-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/leveldbDatabase.h"

/**
 * @brief Construct a new Database object
 * 
 * @param dbName the path of the db file
 */
LeveldbDatabase::LeveldbDatabase(std::string dbName) {
    this->OpenDB(dbName);
}

/**
 * @brief execute query over database
 * 
 * @param key key
 * @param value value
 * @return true success
 * @return false fail
 */
bool LeveldbDatabase::Query(const std::string& key, std::string& value) {
    leveldb::Status queryStatus = this->levelDBObj_->Get(leveldb::ReadOptions(), key, &value);
    return queryStatus.ok();
}


/**
 * @brief insert the (key, value) pair
 * 
 * @param key key
 * @param value value
 * @return true success
 * @return false fail
 */
bool LeveldbDatabase::Insert(const std::string& key, const std::string& value) {
    leveldb::Status insertStatus = this->levelDBObj_->Put(leveldb::WriteOptions(), key, value); 
    return insertStatus.ok();
}

/**
 * @brief open a database
 * 
 * @param dbName the db path 
 * @return true success
 * @return false fail
 */
bool LeveldbDatabase::OpenDB(std::string dbName) {

    // check whether there exists a lock for the given database
    fstream dbLock;
    dbLock.open("." + dbName + ".lock", std::fstream::in);
    if (dbLock.is_open()) {
        dbLock.close();
        fprintf(stderr, "Database lock.\n");
        return false;
    }
    dbName_ = dbName;

    options_.create_if_missing = true;
    options_.block_cache = leveldb::NewLRUCache(256 * 1024 * 1024); // 256MB cache
    leveldb::Status status = leveldb::DB::Open(options_, dbName, &this->levelDBObj_);
    assert(status.ok());
    if (status.ok()) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Destroy the Database:: Database object
 * 
 */
LeveldbDatabase::~LeveldbDatabase() {
    string name = "." + dbName_ + ".lock";
    remove(name.c_str());
    delete levelDBObj_;
    delete options_.block_cache;
}


/**
 * @brief insert the (key, value) pair
 * 
 * @param key 
 * @param buffer 
 * @param bufferSize 
 * @return true 
 * @return false 
 */
bool LeveldbDatabase::InsertBuffer(const std::string& key, const char* buffer, size_t bufferSize) {
    leveldb::Status insertStatus = this->levelDBObj_->Put(leveldb::WriteOptions(),
        key, leveldb::Slice(buffer, bufferSize));
    return insertStatus.ok();
}



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
bool LeveldbDatabase::InsertBothBuffer(const char* key, size_t keySize, const char* buffer,
    size_t bufferSize) {
    leveldb::Status insertStatus = this->levelDBObj_->Put(leveldb::WriteOptions(),
        leveldb::Slice(key, keySize), leveldb::Slice(buffer, bufferSize));
    return insertStatus.ok();
}

/**
 * @brief query the (key, value) pair
 * 
 * @param key 
 * @param keySize 
 * @param value 
 * @return true 
 * @return false 
 */
bool LeveldbDatabase::QueryBuffer(const char* key, size_t keySize, std::string& value) {
    leveldb::Status queryStatus = this->levelDBObj_->Get(leveldb::ReadOptions(),
        leveldb::Slice(key, keySize), &value);
    return queryStatus.ok();
}