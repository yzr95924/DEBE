/**
 * @file dupLESSKM.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the inteface of DupLESS key manager
 * @version 0.1
 * @date 2021-12-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef DUPLEE_KM_H
#define DUPLEE_KM_H

#include "absKM.h"

// for blind RSA
#include <openssl/rsa.h>
#include <openssl/pem.h>

class DupLESSKeyManager : public AbsKeyManager {
    private:
        string myName_ = "DupLESSKM";

        const BIGNUM* serverKeyN_;
        const BIGNUM* serverKeyD_;
        RSA* serverRSA_;
        BIO* privateKeyFile_;
        BN_CTX* bnCTX_;
        BIGNUM* result_;

    public:
        /**
         * @brief Construct a new DupLESS Key Manager object
         * 
         * @param keyManagerChannel the channel for key generation
         */
        DupLESSKeyManager(SSLConnection* keyManagerChannel);

        /**
         * @brief Destroy the DupLESS Key Manager object
         * 
         */
        ~DupLESSKeyManager();

        /**
         * @brief the main thread
         * 
         * @param keyClientSSL the client ssl
         */
        void Run(SSL* keyClientSSL);
};

#endif