/**
 * @file keyECall.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the session key exchange interface
 * @version 0.1
 * @date 2021-09-14
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef KEY_ECALL_H
#define KEY_ECALL_H

#include "commonEnclave.h"

// for the enclave only
#include "ecallEnc.h"
#include "openssl/ec.h"
#include "openssl/ecdh.h"

/**
 * @brief perform the session key exchange
 * 
 * @param publicKeyBuffer the buffer to the peer public key
 * @param clientID the client ID
 */
void Ecall_Session_Key_Exchange(uint8_t* publicKeyBuffer, uint32_t clientID);

/**
 * @brief extract the corresponding public key from the private key
 * 
 * @param privateKey the input private key
 * @return EVP_PKEY* the public key
 */
EVP_PKEY* ExtractPublicKey(EVP_PKEY* privateKey);

/**
 * @brief generate the key pair based on NIST named curve prime256v1 
 * 
 * @return EVP_PKEY* the private key 
 */
EVP_PKEY* GenerateKeyPair();

/**
 * @brief derive the shared secret 
 * 
 * @param publicKey the input peer public key
 * @param privateKey the input private key
 * @return DerivedKey_t* 
 */
DerivedKey_t* DerivedShared(EVP_PKEY* publicKey, EVP_PKEY* privateKey);

#endif