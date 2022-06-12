/**
 * @file absIndex.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in abs index 
 * @version 0.1
 * @date 2020-11-30
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/absIndex.h"

/**
 * @brief Construct a new Abs Index object
 * 
 * @param indexStore the pointer to the index store 
 */
AbsIndex::AbsIndex(AbsDatabase* indexStore) {
    indexStore_ = indexStore;
}

/**
 * @brief Destroy the Abs Index object
 * 
 */
AbsIndex::~AbsIndex() {
}

/**
 * @brief read the information from the index store
 * 
 * @param key key 
 * @param value value
 * @return true success
 * @return false fail
 */
bool AbsIndex::ReadIndexStore(const string& key, string& value) {
    return indexStore_->Query(key, value);
}


/**
 * @brief update the index store
 * 
 * @param key key
 * @param value value 
 * @return true success 
 * @return false fail
 */
bool AbsIndex::UpdateIndexStore(const string& key, const string& value) {
    return indexStore_->Insert(key, value);
}


/**
 * @brief update the index store
 * 
 * @param key key
 * @param buffer the pointer to the buffer
 * @param bufferSize buffer size
 * @return true success
 * @return false fail
 */
bool AbsIndex::UpdateIndexStore(const string& key, const char* buffer, size_t bufferSize) {
    return indexStore_->InsertBuffer(key, buffer, bufferSize);
}