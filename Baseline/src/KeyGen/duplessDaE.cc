/**
 * @file duplessDaE.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of DupLESS DaE
 * @version 0.1
 * @date 2021-12-14
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/duplessDaE.h"

/**
 * @brief Construct a new DupLESSDAE object
 * 
 */
DupLESSDAE::DupLESSDAE(uint8_t* fileNameHash) : AbsDAE(fileNameHash) {
    keyManagerChannel_ = new SSLConnection(config.GetKeyServerIP(), config.GetKeyServerPort(), 
        IN_CLIENTSIDE);
    keyChannelRecord_ = keyManagerChannel_->ConnectSSL();

    // init the send blinded FP buffer
    sendBlindFPBuf_.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendChunkBatchSize_ * RSA_KEY_SIZE);
    sendBlindFPBuf_.header = (NetworkHead_t*) sendBlindFPBuf_.sendBuffer;
    sendBlindFPBuf_.header->clientID = config.GetClientID();
    sendBlindFPBuf_.header->currentItemNum = 0;
    sendBlindFPBuf_.header->dataSize = 0;
    sendBlindFPBuf_.dataBuffer = sendBlindFPBuf_.sendBuffer + sizeof(NetworkHead_t);

    BIO* publicKeyFile = BIO_new_file(KEYMANGER_PUBLIC_KEY_FILE, "r");
    if (!publicKeyFile) {
        fprintf(stderr, "DupLESSDAE: Cannot open the key manager public key file.\n");
        exit(EXIT_FAILURE);
    }
    bnCTX_ = BN_CTX_new();
    publicKey_ = PEM_read_bio_PUBKEY(publicKeyFile, NULL, NULL, NULL);
    clientRSA_ = EVP_PKEY_get1_RSA(publicKey_);
    RSA_get0_key(clientRSA_, &clientKeyN_, &clientKeyE_, NULL);
    BIO_free_all(publicKeyFile);
    tool::Logging(myName_.c_str(), "init the DupLESS-DAE.\n");
}

/**
 * @brief Destroy the DupLESSDAE object
 * 
 */
DupLESSDAE::~DupLESSDAE() {
    delete keyManagerChannel_;
    free(sendBlindFPBuf_.sendBuffer);
    BN_CTX_free(bnCTX_);
    RSA_free(clientRSA_);
    EVP_PKEY_free(publicKey_);
}

/**
 * @brief process one segment of chunk
 * 
 * @param sendPlainChunkBuf the buffer to the plaintext chunk
 * @param sendCipherChunkBuf the buffer to the ciphertext chunk
 * @param sendRecipeBuf the buffer for the recipe <only store the key>
 */
void DupLESSDAE::ProcessBatchChunk(SendMsgBuffer_t* sendPlainChunkBuf,
    SendMsgBuffer_t* sendCipherChunkBuf, SendMsgBuffer_t* sendRecipeBuf) {
    uint32_t chunkNum = sendPlainChunkBuf->header->currentItemNum;
    size_t offset = 0;
    
    uint32_t chunkSize;
    uint8_t* chunkData;
    uint8_t tmpChunkHash[CHUNK_HASH_SIZE] = {0};
    vector<BIGNUM*> invVector;
    for (uint32_t i = 0; i < chunkNum; i++) {
        memcpy(&chunkSize, sendPlainChunkBuf->dataBuffer + offset, 
            sizeof(uint32_t));
        offset += sizeof(uint32_t);
        chunkData = sendPlainChunkBuf->dataBuffer + offset;
        offset += chunkSize;

        // compute the plain chunk fp
        cryptoObj_->GenerateHash(mdCtx_, chunkData, chunkSize, tmpChunkHash);

        // blind-RSA, pick a random
        BIGNUM* r = BN_new();
        BIGNUM* inv = BN_new();
        BN_pseudo_rand(r, 256, -1, 0);
        BN_mod_inverse(inv, r, clientKeyN_, bnCTX_);
        invVector.push_back(inv);

        // decorate the chunk fp
        DecorateFP(r, tmpChunkHash, sendBlindFPBuf_.dataBuffer + 
            sendBlindFPBuf_.header->dataSize);
        sendBlindFPBuf_.header->dataSize += RSA_KEY_SIZE;
        sendBlindFPBuf_.header->currentItemNum++;
        BN_free(r);
    }

    sendBlindFPBuf_.header->messageType = CLIENT_KEY_GEN;
    if (!keyManagerChannel_->SendData(keyChannelRecord_.second, sendBlindFPBuf_.sendBuffer, 
        sizeof(NetworkHead_t) + sendBlindFPBuf_.header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the key gen request error.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t recvSize = 0;
    if (!keyManagerChannel_->ReceiveData(keyChannelRecord_.second, sendBlindFPBuf_.sendBuffer, 
        recvSize)) {
        tool::Logging(myName_.c_str(), "recv the key gen response error.\n");
        exit(EXIT_FAILURE);
    }
    if (sendBlindFPBuf_.header->messageType != KEY_MANAGER_KEY_GEN_REPLY) {
        tool::Logging(myName_.c_str(), "wrong key manager reply type.\n");
        exit(EXIT_FAILURE);
    }

    offset = 0;
    uint8_t key[CHUNK_HASH_SIZE] = {0};
    uint8_t tmpRSAKey[RSA_KEY_SIZE] = {0};
    KeyRecipeEntry_t* tmpRecipe = (KeyRecipeEntry_t*) (sendRecipeBuf->dataBuffer + 
        sendRecipeBuf->header->dataSize);
    for (uint32_t i = 0; i < chunkNum; i++) {
        memcpy(&chunkSize, sendPlainChunkBuf->dataBuffer + offset, 
            sizeof(uint32_t));
        offset += sizeof(uint32_t);
        chunkData = sendPlainChunkBuf->dataBuffer + offset;
        offset += chunkSize;

        Elimination(invVector[i], sendBlindFPBuf_.dataBuffer + i * RSA_KEY_SIZE, 
            tmpRSAKey);
        cryptoObj_->GenerateHash(mdCtx_, tmpRSAKey, RSA_KEY_SIZE, 
            key);
        keyRecipeFile_.write((char*)key, CHUNK_HASH_SIZE);
        memcpy(tmpRecipe->key, key, CHUNK_HASH_SIZE);
        tmpRecipe++;

        // encrypt this chunk to the cipher chunk buffer
        memcpy(sendCipherChunkBuf->dataBuffer + sendCipherChunkBuf->header->dataSize, 
            &chunkSize, sizeof(uint32_t));
        sendCipherChunkBuf->header->dataSize += sizeof(uint32_t);
        cryptoObj_->EncryptWithKey(cipherCtx_, chunkData, chunkSize, key, 
            sendCipherChunkBuf->dataBuffer + sendCipherChunkBuf->header->dataSize);
        sendCipherChunkBuf->header->dataSize += chunkSize;
        sendCipherChunkBuf->header->currentItemNum++;
    }

    // reset the sendBlindFPBuf
    sendBlindFPBuf_.header->dataSize = 0;
    sendBlindFPBuf_.header->currentItemNum = 0;

    // clear all tmp inv
    for (auto it : invVector) {
        BN_free(it);
    }

    return ;
}

/**
 * @brief close the connection with key manager
 * 
 */
void DupLESSDAE::CloseKMConnection() {
    keyManagerChannel_->Finish(keyChannelRecord_);
    return ;
}

 /**
 * @brief decorate the FP
 * 
 * @param r 
 * @param fp the input fp
 * @param outputBuffer the output buffer
 */
void DupLESSDAE::DecorateFP(BIGNUM* r, uint8_t* fp, uint8_t* outputBuffer) {
    BIGNUM* tmp =BN_new();
    BIGNUM* h = BN_new();
    char result[RSA_KEY_SIZE];
    memset(result, 0, sizeof(result));
    BN_bin2bn(fp , 32, h);
    //tmp=hash*r^e mod n
    BN_mod_exp(tmp, r, clientKeyE_, clientKeyN_, bnCTX_);
    BN_mod_mul(tmp, h, tmp, clientKeyN_, bnCTX_);
    BN_bn2bin(tmp, (unsigned char*)result + (RSA_KEY_SIZE - BN_num_bytes(tmp)));
    memcpy(outputBuffer, result, RSA_KEY_SIZE);
    BN_free(tmp);
    BN_free(h);
    return ;
}

/**
 * @brief eliminate the FP
 * 
 * @param inv
 * @param key 
 * @param output 
 */
void DupLESSDAE::Elimination(BIGNUM* inv, uint8_t* key, uint8_t* output) {
    BIGNUM* tmp = BN_new();
    BIGNUM* h = BN_new();
    char result[RSA_KEY_SIZE];
    memset(result, 0, sizeof(result));
    BN_bin2bn(key, RSA_KEY_SIZE, h);
    //tmp=key*r^-1 mod n
    BN_mod_mul(tmp, h, inv, clientKeyN_, bnCTX_);
    BN_bn2bin(tmp, (unsigned char*)result + (RSA_KEY_SIZE - BN_num_bytes(tmp)));
    string resultStr(result, RSA_KEY_SIZE);
    BN_free(tmp);
    BN_free(h);
    memcpy(output, result, RSA_KEY_SIZE);
    return ;
}