/**
 * @file ecallEnc.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the encryption tool inside the enclave 
 * @version 0.1
 * @date 2020-12-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef ECALL_ENC_H
#define ECALL_ENC_H

#include "openssl/evp.h"
#include "openssl/crypto.h"
#include "string.h"

#include "../../../include/chunkStructure.h"
#include "../../../include/constVar.h"

#include "commonEnclave.h"

static const unsigned char ecall_gcm_aad[] = {
    0x4d, 0x23, 0xc3, 0xce, 0xc3, 0x34, 0xb4, 0x9b, 0xdb, 0x37, 0x0c, 0x43,
    0x7f, 0xec, 0x78, 0xde
};


class EcallCrypto {
    private:
        // the type of cipher
        ENCRYPT_SET cipherType_;
        // the type of hash
        HASH_SET hashType_;

        // initialized vector
        uint8_t* iv_;

        /**
         * @brief revise the buffer
         * 
         * @param start the start buffer 
         * @param size the size of the buffer
         */
        inline void reverseBytes(uint8_t* start, int size) {
            unsigned char *lo = start;
            unsigned char *hi = start + size - 1;
            unsigned char swap;
            while (lo < hi) {
                swap = *lo;
                *lo++ = *hi;
                *hi-- = swap;
            }
        }

    public:
        /**
         * @brief Construct a new Ecall Crypto object
         * 
         * @param cipherType cipher type
         * @param hashType hasher type
         */
        EcallCrypto(int cipherType, int hashType);

        /**
         * @brief Destroy the Ecall Crypto object
         * 
         */
        ~EcallCrypto();

        /**
         * @brief generate the hash of the input data
         * 
         * @param mdCtx hasher ctx
         * @param dataBuffer input data buffer
         * @param dataSize input data size
         * @param hash the result hash
         */
        void GenerateHash(EVP_MD_CTX* mdCtx, uint8_t* dataBuffer, const int dataSize, uint8_t* hash);

        /**
         * @brief Encrypt the data with the encryption key 
         * 
         * @param ctx cipher ctx
         * @param dataBuffer input data buffer
         * @param dataSize input data size 
         * @param key encryption key 
         * @param ciphertext output cipherText 
         */
        void EncryptWithKey(EVP_CIPHER_CTX* ctx, uint8_t* dataBuffer, const int dataSize, 
            uint8_t* key, uint8_t* ciphertext);

        /**
         * @brief Decrypt the ciphertext with the encryption key
         * 
         * @param ctx cipher ctx
         * @param ciphertext ciphertext data buffer
         * @param dataSize input data size
         * @param key encryption key 
         * @param dataBuffer output plaintext data 
         */
        void DecryptWithKey(EVP_CIPHER_CTX* ctx, uint8_t* ciphertext, const int dataSize, 
            uint8_t* key, uint8_t* dataBuffer);

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
        void EncryptWithKeyIV(EVP_CIPHER_CTX* ctx, uint8_t* dataBuffer, const int dataSize,
            uint8_t* key, uint8_t* ciphertext, uint8_t* iv);

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
        void DecryptionWithKeyIV(EVP_CIPHER_CTX* ctx, uint8_t* ciphertext, const int dataSize,
            uint8_t* key, uint8_t* dataBuffer, uint8_t* iv);

        /**
         * @brief encrypt for secure communication
         * 
         * @param ctx cipher ctx 
         * @param dataBuffer input data buffer
         * @param dataSize input data size
         * @param key session key
         * @param ciphertext output ciphertext
         */
        void SessionKeyEnc(EVP_CIPHER_CTX* ctx, uint8_t* dataBuffer, const int dataSize, 
            uint8_t* sessionKey, uint8_t* ciphertext);

        /**
         * @brief decrypt for secure communication
         * 
         * @param ctx cipher ctx
         * @param ciphertext ciphertext data buffer
         * @param dataSize input data size
         * @param sessionKey session key
         * @param dataBuffer output plaintext 
         */
        void SessionKeyDec(EVP_CIPHER_CTX* ctx, uint8_t* ciphertext, const int dataSize, 
            uint8_t* sessionKey, uint8_t* dataBuffer);
        
        /**
         * @brief encrypt the index with AES-CMC
         * 
         * @param ctx cipher ctx
         * @param dataBuffer input data buffer
         * @param dataSize input data size
         * @param key the key 
         * @param ciphertext output cipher
         */
        void IndexAESCMCEnc(EVP_CIPHER_CTX* ctx, uint8_t* dataBuffer, const int dataSize,
            uint8_t* key, uint8_t* ciphertext);

        /**
         * @brief decrypt the index with AES-CMC
         * 
         * @param ctx cipher ctx
         * @param ciphertext ciphertext data buffer
         * @param dataSize input data size
         * @param key the key
         * @param dataBuffer output plaintext
         */
        void IndexAESCMCDec(EVP_CIPHER_CTX* ctx, uint8_t* ciphertext, const int dataSize,
            uint8_t* key, uint8_t* dataBuffer);
        
        /**
         * @brief encrypt with AES-CBC-256
         * 
         * @param ctx cipher ctx
         * @param dataBuffer input data buffer
         * @param dataSize input data size
         * @param key the key
         * @param ciphertext output ciphertext
         */
        void AESCBCEnc(EVP_CIPHER_CTX* ctx, uint8_t* dataBuffer, const int dataSize,
            uint8_t* key, uint8_t* ciphertext);

        /**
         * @brief decrypt with AES-CBC-256
         * 
         * @param ctx cipher ctx
         * @param ciphertext ciphertext data buffer
         * @param dataSize input data size
         * @param key the key
         * @param dataBuffer output plaintext
         */
        void AESCBCDec(EVP_CIPHER_CTX* ctx, uint8_t* ciphertext, const int dataSize,
            uint8_t* key, uint8_t* dataBuffer);
};


#endif