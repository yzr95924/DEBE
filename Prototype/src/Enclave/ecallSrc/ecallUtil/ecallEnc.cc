/**
 * @file ecallEnc.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of crypto inside the enclave 
 * @version 0.1
 * @date 2020-12-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/ecallEnc.h"

/**
 * @brief Construct a new Ecall Crypto:: Ecall Crypto object
 * 
 * @param cipherType cipher type
 * @param hashType hash type
 */
EcallCrypto::EcallCrypto(int cipherType, int hashType) {

    // for openssl optimization
    OPENSSL_init_crypto(0, NULL);

    // configure the type of cipher and hasher
    cipherType_ = static_cast<ENCRYPT_SET>(cipherType);
    hashType_ = static_cast<HASH_SET>(hashType);
    
    // prepare buffer
    iv_ = (uint8_t*) malloc(sizeof(uint8_t) * CRYPTO_BLOCK_SIZE);
    if (!iv_) {
        Ocall_SGX_Exit_Error("EcallCrypto: allocate iv fails");
    }
    memset(iv_, 0, sizeof(uint8_t) * CRYPTO_BLOCK_SIZE);
}


/**
 * @brief Destroy the Ecall Crypto object
 * 
 */
EcallCrypto::~EcallCrypto() {
    if (iv_) {
        free(iv_);
    }
}

/**
 * @brief generate the hash of the input data
 * 
 * @param mdCtx hasher ctx
 * @param dataBuffer input data buffer
 * @param dataSize input data size
 * @param hash the result hash
 */
void EcallCrypto::GenerateHash(EVP_MD_CTX* mdCtx, uint8_t* dataBuffer, const int dataSize, uint8_t* hash) {
    int expectedHashSize = 0;
    switch (hashType_) {
        case SHA_1:
            if (!EVP_DigestInit_ex(mdCtx, EVP_sha1(), NULL)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Hasher init error");
            }
            expectedHashSize = 20;
            break;
        case SHA_256:
            if (!EVP_DigestInit_ex(mdCtx, EVP_sha256(), NULL)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Hasher init error");
            }
            expectedHashSize = 32;
            break;
        case MD5:
            if (!EVP_DigestInit_ex(mdCtx, EVP_md5(), NULL)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Hasher init error");
            }
            expectedHashSize = 16;
            break;
    }
    if (!EVP_DigestUpdate(mdCtx, dataBuffer, dataSize)) {
        Ocall_SGX_Exit_Error("EcallCrypto: Hash error");
    }
    uint32_t hashSize;
    if (!EVP_DigestFinal_ex(mdCtx, hash, &hashSize)) {
        Ocall_SGX_Exit_Error("EcallCrypto: Hash error");
    }

    if (hashSize != expectedHashSize) {
        Ocall_SGX_Exit_Error("EcallCrypto: Hash error");;
    }
    
    EVP_MD_CTX_reset(mdCtx);
    return ;
}


/**
 * @brief Encrypt the data with the encryption key 
 * 
 * @param ctx cipher ctx
 * @param dataBuffer input data buffer
 * @param dataSize input data size 
 * @param key encryption key 
 * @param ciphertext output cipherText 
 */
void EcallCrypto::EncryptWithKey(EVP_CIPHER_CTX* ctx, uint8_t* dataBuffer, const int dataSize, 
    uint8_t* key, uint8_t* ciphertext) {
    int cipherLen = 0;
    int len = 0;

    switch (cipherType_) {
        case AES_128_CFB: 
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_cfb(), NULL,
                key, iv_)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Cipher init error");
            }
            break;
        case AES_256_CFB:
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cfb(), NULL,
                key, iv_)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Cipher init error");
            }
            break;
        case AES_256_GCM:
            EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_EncryptInit_ex(ctx, NULL, NULL,
                key, iv_)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Cipher init error");
            }
            EVP_EncryptUpdate(ctx, NULL, &cipherLen, ecall_gcm_aad, sizeof(ecall_gcm_aad));
            break;
        case AES_128_GCM:
            EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_EncryptInit_ex(ctx, NULL, NULL,
                key, iv_)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Cipher init error");
            }
            EVP_EncryptUpdate(ctx, NULL, &cipherLen, ecall_gcm_aad, sizeof(ecall_gcm_aad));
            break;
    }

    // encrypt the plaintext
    if (!EVP_EncryptUpdate(ctx, ciphertext, &cipherLen, dataBuffer, 
        dataSize)) {
        Ocall_SGX_Exit_Error("EcallCrypto: Encryption error");
    }

    EVP_EncryptFinal_ex(ctx, ciphertext + cipherLen, &len);

    cipherLen += len;
	
    if (cipherLen != dataSize) {
        Ocall_SGX_Exit_Error("EcallCrypto: encryption output size not equal to origin size");
    }

    EVP_CIPHER_CTX_reset(ctx);
    return ;
}


/**
 * @brief Decrypt the ciphertext with the encryption key
 * 
 * @param ctx cipher ctx
 * @param ciphertext ciphertext data buffer
 * @param dataSize input data size
 * @param key encryption key 
 * @param dataBuffer output ciphertext 
 */
void EcallCrypto::DecryptWithKey(EVP_CIPHER_CTX* ctx, uint8_t* ciphertext, const int dataSize, 
            uint8_t* key, uint8_t* dataBuffer) {
    int plainLen;
    int len;
    switch (cipherType_) {
        case AES_128_CFB:
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_cfb(), NULL, 
                key, iv_)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Init error");;
            }
            break;
        case AES_256_CFB:
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cfb(), NULL,
                key, iv_)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Init error");;
            }
            break;
        case AES_128_GCM:
            EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_DecryptInit_ex(ctx, NULL, NULL,
                key, iv_)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Init error");
            }
            EVP_DecryptUpdate(ctx, NULL, &plainLen, ecall_gcm_aad, sizeof(ecall_gcm_aad));
            break;
        case AES_256_GCM:
            EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_DecryptInit_ex(ctx, NULL, NULL,
                key, iv_)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Init error");
            }
            EVP_DecryptUpdate(ctx, NULL, &plainLen, ecall_gcm_aad, sizeof(ecall_gcm_aad));
            break;
    }

    // decrypt the plaintext
    if (!EVP_DecryptUpdate(ctx, dataBuffer, &plainLen, ciphertext, 
        dataSize)) {
        Ocall_SGX_Exit_Error("EcallCrypto: Decrypt error");
    }

    EVP_DecryptFinal_ex(ctx, dataBuffer + plainLen, &len);
    plainLen += len;

    if (plainLen != dataSize) {
        Ocall_SGX_Exit_Error("EcallCrypto: Decrypt output size not equal to origin size");
    }

    EVP_CIPHER_CTX_reset(ctx);
    return ;
}

/**
 * @brief encrypt for secure communication
 * 
 * @param ctx cipher ctx 
 * @param dataBuffer input data buffer
 * @param dataSize input data size
 * @param key session key
 * @param ciphertext output ciphertext
 */
void EcallCrypto::SessionKeyEnc(EVP_CIPHER_CTX* ctx, uint8_t* dataBuffer, const int dataSize, 
    uint8_t* sessionKey, uint8_t* ciphertext) {
    int cipherLen = 0;
    int len = 0;

    // for SSL/TLS
    EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
    if (!EVP_EncryptInit_ex(ctx, NULL, NULL,
        sessionKey, iv_)) {
        Ocall_SGX_Exit_Error("CryptoTool: Init error.\n");
    }
    EVP_EncryptUpdate(ctx, NULL, &cipherLen, ecall_gcm_aad, sizeof(ecall_gcm_aad));

    // encrypt the plaintext
    if (!EVP_EncryptUpdate(ctx, ciphertext, &cipherLen, dataBuffer,
        dataSize)) {
        Ocall_SGX_Exit_Error("CryptoTool: Encryption error.\n");
    }
    EVP_EncryptFinal_ex(ctx, ciphertext + cipherLen, &len);
    cipherLen += len;

    if (cipherLen != dataSize) {
        Ocall_SGX_Exit_Error("CryptoTool: encryption output size not equal to original size.\n");
    }

    EVP_CIPHER_CTX_reset(ctx);
    return ;
}

/**
 * @brief decrypt for secure communication
 * 
 * @param ctx cipher ctx
 * @param ciphertext ciphertext data buffer
 * @param dataSize input data size
 * @param sessionKey session key
 * @param dataBuffer output plaintext 
 */
void EcallCrypto::SessionKeyDec(EVP_CIPHER_CTX* ctx, uint8_t* ciphertext, const int dataSize, 
    uint8_t* sessionKey, uint8_t* dataBuffer) {
    int plainLen;
    int len;
    
    // follow SSL/TLS
    EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
    if (!EVP_DecryptInit_ex(ctx, NULL, NULL,
        sessionKey, iv_)) {
        Ocall_SGX_Exit_Error("CryptoTool: Init error.\n");
    }
    EVP_DecryptUpdate(ctx, NULL, &plainLen, ecall_gcm_aad, sizeof(ecall_gcm_aad));

    // decrypt the ciphertext 
    if (!EVP_DecryptUpdate(ctx, dataBuffer, &plainLen, ciphertext, 
        dataSize)) {
        Ocall_SGX_Exit_Error("CryptoTool: Decrypt error.\n");
    } 
    EVP_DecryptFinal_ex(ctx, dataBuffer + plainLen, &len);
    plainLen += len;

    if (plainLen != dataSize) {
        Ocall_SGX_Exit_Error("CryptoTool: Decrypt output size not equal to origin size.\n");
    }

    EVP_CIPHER_CTX_reset(ctx);
    return ;
}

/**
 * @brief Encrypt the data with encryption key and iv
 * 
 * @param ctx cipher ctx
 * @param dataBuffer input data buffer
 * @param dataSize input data size
 * @param key encryption key
 * @param ciphertext output ciphertext
 * @param iv the iv
 */
void EcallCrypto::EncryptWithKeyIV(EVP_CIPHER_CTX* ctx, uint8_t* dataBuffer, const int dataSize,
    uint8_t* key, uint8_t* ciphertext, uint8_t* iv) {
    int cipherLen = 0;
    int len = 0;

    switch (cipherType_) {
        case AES_128_CFB: 
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_cfb(), NULL,
                key, iv)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Cipher init error");
            }
            break;
        case AES_256_CFB:
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cfb(), NULL,
                key, iv)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Cipher init error");
            }
            break;
        case AES_256_GCM:
            EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_EncryptInit_ex(ctx, NULL, NULL,
                key, iv)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Cipher init error");
            }
            EVP_EncryptUpdate(ctx, NULL, &cipherLen, ecall_gcm_aad, sizeof(ecall_gcm_aad));
            break;
        case AES_128_GCM:
            EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_EncryptInit_ex(ctx, NULL, NULL,
                key, iv)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Cipher init error");
            }
            EVP_EncryptUpdate(ctx, NULL, &cipherLen, ecall_gcm_aad, sizeof(ecall_gcm_aad));
            break;
    }

    // encrypt the plaintext
    if (!EVP_EncryptUpdate(ctx, ciphertext, &cipherLen, dataBuffer, 
        dataSize)) {
        Ocall_SGX_Exit_Error("EcallCrypto: Encryption error");
    }

    EVP_EncryptFinal_ex(ctx, ciphertext + cipherLen, &len);

    cipherLen += len;
	
    if (cipherLen != dataSize) {
        Ocall_SGX_Exit_Error("EcallCrypto: encryption output size not equal to origin size");
    }

    EVP_CIPHER_CTX_reset(ctx);
    return ;
}

/**
 * @brief Decrypt the ciphertext with the encryption key and iv
 * 
 * @param ctx cipher ctx
 * @param ciphertext input data buffer
 * @param dataSize input data size
 * @param key encryption key
 * @param dataBuffer output plaintext data
 * @param iv the iv
 */
void EcallCrypto::DecryptionWithKeyIV(EVP_CIPHER_CTX* ctx, uint8_t* ciphertext, const int dataSize,
    uint8_t* key, uint8_t* dataBuffer, uint8_t* iv) {
    int plainLen;
    int len;
    switch (cipherType_) {
        case AES_128_CFB:
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_cfb(), NULL, 
                key, iv)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Init error");;
            }
            break;
        case AES_256_CFB:
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cfb(), NULL,
                key, iv)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Init error");;
            }
            break;
        case AES_128_GCM:
            EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_DecryptInit_ex(ctx, NULL, NULL,
                key, iv)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Init error");
            }
            EVP_DecryptUpdate(ctx, NULL, &plainLen, ecall_gcm_aad, sizeof(ecall_gcm_aad));
            break;
        case AES_256_GCM:
            EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_DecryptInit_ex(ctx, NULL, NULL,
                key, iv)) {
                Ocall_SGX_Exit_Error("EcallCrypto: Init error");
            }
            EVP_DecryptUpdate(ctx, NULL, &plainLen, ecall_gcm_aad, sizeof(ecall_gcm_aad));
            break;
    }

    // decrypt the plaintext
    if (!EVP_DecryptUpdate(ctx, dataBuffer, &plainLen, ciphertext, 
        dataSize)) {
        Ocall_SGX_Exit_Error("EcallCrypto: Decrypt error");
    }

    EVP_DecryptFinal_ex(ctx, dataBuffer + plainLen, &len);
    plainLen += len;

    if (plainLen != dataSize) {
        Ocall_SGX_Exit_Error("EcallCrypto: Decrypt output size not equal to origin size");
    }

    EVP_CIPHER_CTX_reset(ctx);
    return ;
}

/**
 * @brief encrypt with AES-CBC-256
 * 
 * @param ctx cipher ctx
 * @param dataBuffer input data buffer
 * @param dataSize input data size
 * @param key the key
 * @param ciphertext output ciphertext
 */
void EcallCrypto::AESCBCEnc(EVP_CIPHER_CTX* ctx, uint8_t* dataBuffer, const int dataSize,
    uint8_t* key, uint8_t* ciphertext) {
    int cipherLen = 0;
    int len = 0;
    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, 
        iv_)) {
        Ocall_SGX_Exit_Error("EcallCrypto: Init error");
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    if (!EVP_EncryptUpdate(ctx, ciphertext, &cipherLen, dataBuffer, 
        dataSize)) {
        Ocall_SGX_Exit_Error("EcallCrypto: Encryption error");
    }

    EVP_EncryptFinal_ex(ctx, ciphertext + cipherLen, &len);
    cipherLen += len;

    if (cipherLen != dataSize) {
        Ocall_SGX_Exit_Error("EcallCrypto: encryption output size not equal to origin size");
    }

    EVP_CIPHER_CTX_reset(ctx);
    return ;
}

/**
 * @brief decrypt with AES-CBC-256
 * 
 * @param ctx cipher ctx
 * @param ciphertext ciphertext data buffer
 * @param dataSize input data size
 * @param key the key
 * @param dataBuffer output plaintext
 */
void EcallCrypto::AESCBCDec(EVP_CIPHER_CTX* ctx, uint8_t* ciphertext, const int dataSize,
    uint8_t* key, uint8_t* dataBuffer) {
    int plainLen;
    int len;
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
        key, iv_)) {
        Ocall_SGX_Exit_Error("EcallCrypto: Init error");
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    if (!EVP_DecryptUpdate(ctx, dataBuffer, &plainLen, ciphertext, 
        dataSize)) {
        Ocall_SGX_Exit_Error("EcallCrypto: Decrypt error");
    }

    EVP_DecryptFinal_ex(ctx, dataBuffer + plainLen, &len);
    plainLen += len;

    if (plainLen != dataSize) {
        Ocall_SGX_Exit_Error("EcallCrypto: Decrypt output size not equal to origin size");
    }

    EVP_CIPHER_CTX_reset(ctx);
    return ;
}

/**
 * @brief encrypt the index key with AES-CMC
 * 
 * @param ctx cipher ctx
 * @param dataBuffer input data buffer
 * @param dataSize input data size
 * @param key the key 
 * @param ciphertext output cipher
 */
void EcallCrypto::IndexAESCMCEnc(EVP_CIPHER_CTX* ctx, uint8_t* dataBuffer, const int dataSize,
    uint8_t* key, uint8_t* ciphertext) {
    string tmpBuffer(dataSize, 0);
    this->AESCBCEnc(ctx, dataBuffer, dataSize, key, (uint8_t*)&tmpBuffer[0]);
    this->reverseBytes((uint8_t*)&tmpBuffer[0], dataSize);
    this->AESCBCEnc(ctx, (uint8_t*)&tmpBuffer[0], dataSize, key, ciphertext);
    return ;
}

/**
 * @brief decrypt the index with AES-CMC
 * 
 * @param ctx cipher ctx
 * @param ciphertext ciphertext data buffer
 * @param dataSize input data size
 * @param key the key
 * @param dataBuffer output plaintext
 */
void EcallCrypto::IndexAESCMCDec(EVP_CIPHER_CTX* ctx, uint8_t* ciphertext, const int dataSize,
    uint8_t* key, uint8_t* dataBuffer) {
    string tmpBuffer(dataSize, 0);
    this->AESCBCDec(ctx, ciphertext, dataSize, key, (uint8_t*)&tmpBuffer[0]);
    this->reverseBytes((uint8_t*)&tmpBuffer[0], dataSize);
    this->AESCBCDec(ctx, (uint8_t*)&tmpBuffer[0], dataSize, key, dataBuffer);
    return ;
}