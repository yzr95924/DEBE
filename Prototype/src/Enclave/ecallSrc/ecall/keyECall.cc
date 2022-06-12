/**
 * @file keyECall.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief for session key exchange
 * @version 0.1
 * @date 2021-09-14
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/keyECall.h"

EVP_PKEY* GenerateKeyPair() {
    EVP_PKEY_CTX* paramGenCtx = NULL;
    EVP_PKEY_CTX* keyGenCtx = NULL;

    EVP_PKEY* params = NULL;
    EVP_PKEY* keyPair = NULL; // return 

    paramGenCtx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);

    if (!EVP_PKEY_paramgen_init(paramGenCtx)) {
        Ocall_SGX_Exit_Error("GenerateKey: init the param ctx fails.");
    }

    EVP_PKEY_CTX_set_ec_paramgen_curve_nid(paramGenCtx, NID_X9_62_prime256v1);
    EVP_PKEY_paramgen(paramGenCtx, &params);
    keyGenCtx = EVP_PKEY_CTX_new(params, NULL);
    if (!EVP_PKEY_keygen_init(keyGenCtx)) {
        Ocall_SGX_Exit_Error("GenerateKey: key gen init fails.");
    }
    if (!EVP_PKEY_keygen(keyGenCtx, &keyPair)) {
        Ocall_SGX_Exit_Error("GenerateKey: key gen fails.");
    }

    EC_KEY* ecKey = EVP_PKEY_get1_EC_KEY(keyPair);

    const EC_POINT* pubPoint = EC_KEY_get0_public_key(ecKey);
    BIGNUM* x = BN_new();
    BIGNUM* y = BN_new();
    EC_GROUP* tmpECGroup = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    EC_POINT_get_affine_coordinates_GFp(tmpECGroup, pubPoint, x, y, NULL);

    EVP_PKEY_CTX_free(paramGenCtx);
    EVP_PKEY_CTX_free(keyGenCtx);
    BN_free(x);
    BN_free(y);
    EVP_PKEY_free(params);
    EC_KEY_free(ecKey);
    EC_GROUP_free(tmpECGroup);
    
    return keyPair;
}

/**
 * @brief extract the corresponding public key from the private key
 * 
 * @param privateKey the input private key
 * @return EVP_PKEY* the public key
 */
EVP_PKEY* ExtractPublicKey(EVP_PKEY* privateKey) {
    EC_KEY* ecKey = EVP_PKEY_get1_EC_KEY(privateKey);
    const EC_POINT* ecPoint = EC_KEY_get0_public_key(ecKey);
    EVP_PKEY* publicKey = EVP_PKEY_new();
    EC_KEY* pubEcKey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    EC_KEY_set_public_key(pubEcKey, ecPoint);
    EVP_PKEY_set1_EC_KEY(publicKey, pubEcKey);

    EC_KEY_free(pubEcKey);
    EC_KEY_free(ecKey);

    return publicKey;
}

/**
 * @brief derive the shared secret 
 * 
 * @param publicKey the input peer public key
 * @param privateKey the input private key
 * @return DerivedKey_t* 
 */
DerivedKey_t* DerivedShared(EVP_PKEY* publicKey, EVP_PKEY* privateKey) {
    DerivedKey_t* dk = (DerivedKey_t*) malloc(sizeof(DerivedKey_t));
    EVP_PKEY_CTX* derivationCtx = NULL;
    derivationCtx = EVP_PKEY_CTX_new(privateKey, NULL);

    EVP_PKEY_derive_init(derivationCtx);
    EVP_PKEY_derive_set_peer(derivationCtx, publicKey);

    if (1 != EVP_PKEY_derive(derivationCtx, NULL, &dk->length)) {
        Ocall_SGX_Exit_Error("Derive the secret length fails.");
    }

    dk->secret = (uint8_t*) malloc(dk->length);


    if (1 != (EVP_PKEY_derive(derivationCtx, dk->secret, &dk->length))) {
        Ocall_SGX_Exit_Error("Derive the key length fails.");
    }

    EVP_PKEY_CTX_free(derivationCtx);

    return dk; 
}

/**
 * @brief perform the session key exchange
 * 
 * @param publicKeyBuffer the buffer to the peer public key
 * @param clientID the client ID
 */
void Ecall_Session_Key_Exchange(uint8_t* publicKeyBuffer, uint32_t clientID) {
    // generate a random private key
    EVP_PKEY* privateKey = GenerateKeyPair();

    // extract the corresponding public key
    EVP_PKEY* publicKey = ExtractPublicKey(privateKey);

    uint8_t* tmpPubKeyBuffer;
    size_t pubKeyLen = 0;
    pubKeyLen = EVP_PKEY_get1_tls_encodedpoint(publicKey, &tmpPubKeyBuffer);

    EVP_PKEY* peerPubKey = EVP_PKEY_new();
    if (peerPubKey == NULL || EVP_PKEY_copy_parameters(peerPubKey, publicKey) <= 0) {
        Ocall_SGX_Exit_Error("SessionKeyExchange: init the tmp public key fails.");
    }
    EVP_PKEY_set1_tls_encodedpoint(peerPubKey, publicKeyBuffer, pubKeyLen);
    DerivedKey_t* sessionKey = DerivedShared(peerPubKey, privateKey);

    string sessionKeyStr;
    sessionKeyStr.resize(CHUNK_ENCRYPT_KEY_SIZE, 0);
    memcpy(&sessionKeyStr[0], sessionKey->secret, sessionKey->length);

{
    // update the session key index here
#if (MULTI_CLIENT == 1)
    Enclave::sessionKeyLck_.lock();
#endif
    Enclave::clientSessionKeyIndex_[clientID] = sessionKeyStr;
#if (MULTI_CLIENT == 1)
    Enclave::sessionKeyLck_.unlock();
#endif
}
    // write the public buffer to the outside
    memcpy(publicKeyBuffer, tmpPubKeyBuffer, pubKeyLen);

    OPENSSL_free(tmpPubKeyBuffer);
    EVP_PKEY_free(privateKey);
    EVP_PKEY_free(publicKey);
    EVP_PKEY_free(peerPubKey);
    free(sessionKey->secret);
    free(sessionKey);
    return ;
}

