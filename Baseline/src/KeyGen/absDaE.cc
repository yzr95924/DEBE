/**
 * @file absDaE.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of DAE
 * @version 0.1
 * @date 2021-05-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/absDaE.h"

/**
 * @brief Construct a new AbsDAE object
 * 
 * @param fileName the input file name
 */
AbsDAE::AbsDAE(uint8_t* fileNameHash) {
    sendChunkBatchSize_ = config.GetSendChunkBatchSize();
    clientID_ = config.GetClientID();
    mdCtx_ = EVP_MD_CTX_new();
    cipherCtx_ = EVP_CIPHER_CTX_new();
    cryptoObj_ = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);

    char keyRecipeNameBuffer[CHUNK_HASH_SIZE * 2 + 1];
    for (size_t i = 0; i < CHUNK_HASH_SIZE; i++) {
        sprintf(keyRecipeNameBuffer + 2 * i, "%02X", fileNameHash[i]);
    }
    keyRecipeNameStr_.assign(keyRecipeNameBuffer, CHUNK_HASH_SIZE * 2);
    keyRecipeNameStr_ = config.GetRecipeRootPath() + keyRecipeNameStr_ + keyRecipeLocalSuffix_;

    keyRecipeFile_.open(keyRecipeNameStr_.c_str(), ios_base::trunc);
    if (!keyRecipeFile_.is_open()) {
        tool::Logging(myName_.c_str(), "cannot open the key recipe: %s.\n", 
            keyRecipeNameStr_.c_str());
        exit(EXIT_FAILURE);
    }
}   

/**
 * @brief Destroy the Abs DAE object
 * 
 */
AbsDAE::~AbsDAE() {
    EVP_MD_CTX_free(mdCtx_);
    EVP_CIPHER_CTX_free(cipherCtx_);
    delete cryptoObj_;
    keyRecipeFile_.close();
}
