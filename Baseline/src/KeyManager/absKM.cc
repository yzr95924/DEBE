/**
 * @file absKM.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2021-12-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/absKM.h"

/**
 * @brief Construct a new Abs Key Manager object
 * 
 * @param keyManagerChannel the key generation channel
 */
AbsKeyManager::AbsKeyManager(SSLConnection* keyManagerChannel) {
    keyManagerChannel_ = keyManagerChannel;
    sendChunkBatchSize_ = config.GetSendChunkBatchSize();
    memset(globalSecret_, 1, CHUNK_HASH_SIZE);
    cryptoObj_ = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE); 
    mdCTX_ = EVP_MD_CTX_new();
}

/**
 * @brief Destroy the Abs Key Manager:: Abs Key Manager object
 * 
 */
AbsKeyManager::~AbsKeyManager() {
    delete cryptoObj_;
    EVP_MD_CTX_free(mdCTX_);
}