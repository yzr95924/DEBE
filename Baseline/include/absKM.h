/**
 * @file absKM.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of the abs key manager
 * @version 0.1
 * @date 2021-12-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ABS_KM_H
#define ABS_KM_H

#include "configure.h"
#include "sslConnection.h"
#include "cryptoPrimitive.h"

extern Configure config;

class AbsKeyManager {
    protected:
        // handler passed from outside
        SSLConnection* keyManagerChannel_;

        // config 
        uint64_t sendChunkBatchSize_ = 0;

        // for crypto operation
        CryptoPrimitive* cryptoObj_;
        EVP_MD_CTX* mdCTX_;
        uint8_t globalSecret_[CHUNK_HASH_SIZE];

    public:
        // for statistics
        uint64_t _totalKeyGenNum = 0;

        /**
         * @brief Construct a new Abs Key Manager object
         * 
         * @param keyManagerChannel the key generation channel
         */
        AbsKeyManager(SSLConnection* keyManagerChannel);

        /**
         * @brief Destroy the Abs Key Manager object
         * 
         */
        virtual ~AbsKeyManager();

        /**
         * @brief the main process of the client
         * 
         * @param keyClientSSL the client ssl
         */
        virtual void Run(SSL* keyClientSSL) = 0;
};

#endif