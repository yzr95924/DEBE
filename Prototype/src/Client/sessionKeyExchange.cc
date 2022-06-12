/**
 * @file sessionKeyExchange.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief session key exchange based on ECDH
 * @version 0.1
 * @date 2021-09-13
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/sessionKeyExchange.h"

/**
 * @brief Construct a new Session Key Exchange object
 * 
 * @param dataSecureChannel for secure communication
 */
SessionKeyExchange::SessionKeyExchange(SSLConnection* dataSecureChannel) {
    dataSecureChannel_ = dataSecureChannel;
}

/**
 * @brief Destroy the Session Key Exchange object
 * 
 */
SessionKeyExchange::~SessionKeyExchange() {
}

/**
 * @brief generate a key pair
 * 
 * @return EVP_PKEY* the key pair
 */
EVP_PKEY* SessionKeyExchange::GenerateKeyPair() {
    EVP_PKEY_CTX* paramGenCtx = NULL;
    EVP_PKEY_CTX* keyGenCtx = NULL;

    EVP_PKEY* params = NULL;
    EVP_PKEY* keyPair = NULL; // return 

    paramGenCtx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);

    if (!EVP_PKEY_paramgen_init(paramGenCtx)) {
        tool::Logging(myName_.c_str(), "init the param ctx fails.\n");
        exit(EXIT_FAILURE);
    }

    EVP_PKEY_CTX_set_ec_paramgen_curve_nid(paramGenCtx, NID_X9_62_prime256v1);
    EVP_PKEY_paramgen(paramGenCtx, &params);
    keyGenCtx = EVP_PKEY_CTX_new(params, NULL);
    if (!EVP_PKEY_keygen_init(keyGenCtx)) {
        tool::Logging(myName_.c_str(), "key gen init fails.\n");
        exit(EXIT_FAILURE);
    }
    if (!EVP_PKEY_keygen(keyGenCtx, &keyPair)) {
        tool::Logging(myName_.c_str(), "key gen fails.\n");
        exit(EXIT_FAILURE);
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
EVP_PKEY* SessionKeyExchange::ExtractPublicKey(EVP_PKEY* privateKey) {
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
DerivedKey_t* SessionKeyExchange::DerivedShared(EVP_PKEY* publicKey, EVP_PKEY* privateKey) {
    DerivedKey_t* dk = (DerivedKey_t*) malloc(sizeof(DerivedKey_t));
    EVP_PKEY_CTX* derivationCtx = NULL;
    derivationCtx = EVP_PKEY_CTX_new(privateKey, NULL);

    EVP_PKEY_derive_init(derivationCtx);
    EVP_PKEY_derive_set_peer(derivationCtx, publicKey);

    if (1 != EVP_PKEY_derive(derivationCtx, NULL, &dk->length)) {
        tool::Logging(myName_.c_str(), "derive the secret length fails.\n");
        exit(EXIT_FAILURE);
    }

    dk->secret = (uint8_t*) malloc(dk->length);


    if (1 != (EVP_PKEY_derive(derivationCtx, dk->secret, &dk->length))) {
        tool::Logging(myName_.c_str(), "derive the key length fails.\n");
        exit(EXIT_FAILURE);
    }

    EVP_PKEY_CTX_free(derivationCtx);

    return dk; 
}

/**
 * @brief Generate a secret via ECDH
 * 
 * @param secret string the string to store the secret
 * @param serverConnection the connection to the server 
 * @param clientID the client ID
 */
void SessionKeyExchange::GeneratingSecret(uint8_t* secret, SSL* serverConnection, 
    uint32_t clientID) {
    // generate a random private key
    EVP_PKEY* privateKey = GenerateKeyPair();

    // extract the corrresponding public key
    EVP_PKEY* publicKey = ExtractPublicKey(privateKey);

    uint8_t* pubKeyBuffer;
    size_t pubKeyLen = 0;
    pubKeyLen = EVP_PKEY_get1_tls_encodedpoint(publicKey, &pubKeyBuffer);

    NetworkHead_t tmpHeader;
    tmpHeader.messageType = SESSION_KEY_INIT;
    tmpHeader.dataSize = pubKeyLen;
    tmpHeader.clientID = clientID;
    uint8_t* sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + pubKeyLen);

    memcpy(sendBuffer, &tmpHeader, sizeof(NetworkHead_t));
    memcpy(sendBuffer + sizeof(NetworkHead_t), pubKeyBuffer, pubKeyLen);
    if (!dataSecureChannel_->SendData(serverConnection, sendBuffer, sizeof(NetworkHead_t) + pubKeyLen)) {
        tool::Logging(myName_.c_str(), "send the session key init request fails.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t recvSize = 0; 
    if (!dataSecureChannel_->ReceiveData(serverConnection, sendBuffer, recvSize)) {
        tool::Logging(myName_.c_str(), "recv the session key reply fails.\n");
        exit(EXIT_FAILURE);
    }

    // check the recv key
    memcpy(&tmpHeader, sendBuffer, sizeof(NetworkHead_t));
    if (tmpHeader.messageType != SESSION_KEY_REPLY) {
        tool::Logging(myName_.c_str(), "recv the session key reply wrong type.\n");
        exit(EXIT_FAILURE);
    }
    pubKeyLen = tmpHeader.dataSize;

    EVP_PKEY* peerPubKey = EVP_PKEY_new();
    if (peerPubKey == NULL || EVP_PKEY_copy_parameters(peerPubKey, publicKey) <= 0) {
        tool::Logging(myName_.c_str(), "init the tmp public key fails.\n");
        exit(EXIT_FAILURE);
    }
    EVP_PKEY_set1_tls_encodedpoint(peerPubKey, sendBuffer + sizeof(NetworkHead_t), pubKeyLen);

    DerivedKey_t* sessionKey = DerivedShared(peerPubKey, privateKey);
    memcpy(&secret[0], sessionKey->secret, CHUNK_ENCRYPT_KEY_SIZE);

    free(sendBuffer);
    OPENSSL_free(pubKeyBuffer);
    EVP_PKEY_free(privateKey);
    EVP_PKEY_free(publicKey);
    EVP_PKEY_free(peerPubKey);
    free(sessionKey->secret);
    free(sessionKey);
    return ;
}