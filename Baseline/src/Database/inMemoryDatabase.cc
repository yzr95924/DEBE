/**
 * @file inMemoryDatabase.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of in-memory index
 * @version 0.1
 * @date 2021-05-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/inMemoryDatabase.h"


/**
 * @brief Construct a new In Memory Database object
 * 
 * @param dbName the path of the db file
 */
InMemoryDatabase::InMemoryDatabase(std::string dbName) {
    this->OpenDB(dbName);
}

/**
 * @brief Destroy the In Memory Database object
 * 
 */
InMemoryDatabase::~InMemoryDatabase() {
    // perisistent the indexFile to the disk
    ofstream dbFile;
    dbFile.open(dbName_, ios_base::trunc | ios_base::binary);
    int itemSize = 0;
    for (auto it = indexObj_.begin(); it != indexObj_.end(); it++) {
        // write the key
        itemSize = it->first.size();
        dbFile.write((char*)&itemSize, sizeof(itemSize));
        dbFile.write(it->first.c_str(), itemSize);

        // write the value
        itemSize = it->second.size();
        dbFile.write((char*)&itemSize, sizeof(itemSize));
        dbFile.write(it->second.c_str(), itemSize);
    }
    dbFile.close();
}

/**
 * @brief open a database
 * 
 * @param dbName the db path
 * @return true success
 * @return false fails
 */
bool InMemoryDatabase::OpenDB(std::string dbName) {
    dbName_ = dbName;
    // check whether there exists the index
    ifstream dbFile;
    dbFile.open(dbName_, ios_base::in | ios_base::binary);
    if (!dbFile.is_open()) {
        fprintf(stderr, "InMemoryDatabase: cannot open the db file.\n");
    }

    size_t beginSize = dbFile.tellg();
    dbFile.seekg(0, ios_base::end);
    size_t fileSize = dbFile.tellg();
    fileSize = fileSize - beginSize;

    if (fileSize == 0) {
        // db file not exist
        fprintf(stderr, "InMemoryDatabase: db file file is not exists, create a new one.\n");
    } else {
        // db file exist, load
        dbFile.seekg(0, ios_base::beg);
        bool isEnd = false;
        int itemSize = 0;
        string key;
        string value;
        while (!isEnd) {
            // read key
            dbFile.read((char*)&itemSize, sizeof(itemSize));
            if (itemSize == 0) {
                break;
            }
            key.resize(itemSize, 0); 
            dbFile.read((char*)&key[0], itemSize);

            // read value
            dbFile.read((char*)&itemSize, sizeof(itemSize));
            value.resize(itemSize, 0);
            dbFile.read((char*)&value[0], itemSize);
            
            // update the index
            indexObj_.insert(make_pair(key, value));
            itemSize = 0;
            // update the read flag
            isEnd = dbFile.eof();
        }
    }
    dbFile.close();
    fprintf(stderr, "InMemoryDatabase: loaded index size: %lu\n", indexObj_.size());
    return true;
}

/**
 * @brief execute query over database
 * 
 * @param key key
 * @param value value
 * @return true success
 * @return false fail
 */
bool InMemoryDatabase::Query(const std::string& key, std::string& value) {
    auto findResult = indexObj_.find(key);
    if (findResult != indexObj_.end()) {
        // it exists in the index
        value.assign(findResult->second);
        return true;
    } 
    return false;
}


/**
 * @brief insert the (key, value) pair
 * 
 * @param key key
 * @param value value
 * @return true success
 * @return false fail
 */
bool InMemoryDatabase::Insert(const std::string& key, const std::string& value) {
    indexObj_[key] = value;
    return true;
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
bool InMemoryDatabase::InsertBuffer(const std::string& key, const char* buffer, size_t bufferSize) {
    string valueStr;
    valueStr.assign(buffer, bufferSize);
    indexObj_[key] = valueStr;
    return true;
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
bool InMemoryDatabase::InsertBothBuffer(const char* key, size_t keySize, const char* buffer,
    size_t bufferSize) {
    string keyStr;
    string valueStr;
    keyStr.assign(key, keySize);
    valueStr.assign(buffer, bufferSize);
    indexObj_[keyStr] = valueStr;
    return true;
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
bool InMemoryDatabase::QueryBuffer(const char* key, size_t keySize, std::string& value) {
    string keyStr;
    keyStr.assign(key, keySize);
    auto findResult = indexObj_.find(keyStr);
    if (findResult != indexObj_.end()) {
        // it exists in the index
        value.assign(findResult->second);
        return true;
    }
    return false;
}