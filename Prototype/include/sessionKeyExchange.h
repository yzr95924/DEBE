/**
 * @file sessionKeyExchange.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief session key exchange
 * @version 0.1
 * @date 2021-09-13
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef SESSION_KEY_EXCHANGE_H
#define SESSION_KEY_EXCHANGE_H

#include <openssl/ecdh.h>
#include <openssl/ec.h>

#include "cryptoPrimitive.h"
#include "sslConnection.h"

class SessionKeyExchange {
    private:
        string myName_ = "SessionKeyExchange";
        /**
         * @brief generate the key pair based on NIST named curve prime256v1 
         * 
         * @return EVP_PKEY* the private key 
         */
        EVP_PKEY* GenerateKeyPair();

        /**
         * @brief extract the corresponding public key from the private key
         * 
         * @param privateKey the input private key
         * @return EVP_PKEY* the public key
         */
        EVP_PKEY* ExtractPublicKey(EVP_PKEY* privateKey);

        /**
         * @brief derive the shared secret 
         * 
         * @param publicKey the input peer public key
         * @param privateKey the input private key
         * @return DerivedKey_t* 
         */
        DerivedKey_t* DerivedShared(EVP_PKEY* publicKey, EVP_PKEY* privateKey);

        // for communication 
        SSLConnection* dataSecureChannel_;

    public:
        /**
         * @brief Construct a new Session Key Exchange object
         * 
         * @param dataSecureChannel for secure communication
         */
        SessionKeyExchange(SSLConnection* dataSecureChannel);

        /**
         * @brief Destroy the Session Key Exchange object
         * 
         */
        ~SessionKeyExchange();

        /**
         * @brief Generate a secret via ECDH
         * 
         * @param secret string the string to store the secret
         * @param serverConnection the connection to the server 
         * @param clientID the client ID
         */
        void GeneratingSecret(uint8_t* secret, SSL* serverConnection, uint32_t clientID);
};

#endif