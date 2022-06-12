/**
 * @file dupLESSKM.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2021-12-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/dupLESSKM.h"

/**
 * @brief Construct a new DupLESS Key Manager object
 * 
 * @param keyManagerChannel the channel for key generation
 */
DupLESSKeyManager::DupLESSKeyManager(SSLConnection* keyManagerChannel) :
    AbsKeyManager(keyManagerChannel) {
    privateKeyFile_ = BIO_new_file(KEYMANGER_PRIVATE_KEY, "r");
    serverRSA_ = RSA_new();
    char passwd[5] = "1111";
    passwd[4] = '\0';
    PEM_read_bio_RSAPrivateKey(privateKeyFile_, &serverRSA_, NULL, passwd);
    RSA_get0_key(serverRSA_, &serverKeyN_, NULL, &serverKeyD_);
    BIO_free_all(privateKeyFile_);
    
    // finish the init of the blind-RSA
    bnCTX_ = BN_CTX_new();
    result_ = BN_new();
    tool::Logging(myName_.c_str(), "init the DupLESS-KeyManager.\n");
}

/**
 * @brief Destroy the DupLESS Key Manager object
 * 
 */
DupLESSKeyManager::~DupLESSKeyManager() {
    BN_free(result_);
    BN_CTX_free(bnCTX_);
    RSA_free(serverRSA_);
    fprintf(stderr,"========DupLESSKeyManager Info========\n");
    fprintf(stderr,"key gen num: %lu\n", _totalKeyGenNum);
    fprintf(stderr,"======================================\n");
}

/**
 * @brief the main process of the client
 * 
 * @param keyClientSSL the client ssl
 */
void DupLESSKeyManager::Run(SSL* keyClientSSL) {
    uint32_t recvSize = 0;
    string clientIP;
    SendMsgBuffer_t recvBuf;
    recvBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendChunkBatchSize_ * RSA_KEY_SIZE);
    recvBuf.header = (NetworkHead_t*) recvBuf.sendBuffer;
    recvBuf.header->dataSize = 0;
    recvBuf.dataBuffer = recvBuf.sendBuffer + sizeof(NetworkHead_t);
    uint32_t clientID = 0;

    double totalProcessTime = 0;
    struct timeval sProcTime;
    struct timeval eProcTime;

    tool::Logging(myName_.c_str(), "the main thread is running.\n");
    while (true) {
        // recv data
        if (!keyManagerChannel_->ReceiveData(keyClientSSL, recvBuf.sendBuffer, 
            recvSize)) {
            tool::Logging(myName_.c_str(), "client closed the socket connection.\n");
            keyManagerChannel_->GetClientIp(clientIP, keyClientSSL);
            keyManagerChannel_->ClearAcceptedClientSd(keyClientSSL);
            clientID = recvBuf.header->clientID;
            break;
        } else {
            gettimeofday(&sProcTime, NULL);
            if (recvBuf.header->messageType != CLIENT_KEY_GEN) {
                tool::Logging(myName_.c_str(), "wrong key gen req type.\n");
                exit(EXIT_FAILURE);
            }

            // perform blind-RSA
            uint32_t recvRSANum = recvBuf.header->currentItemNum;
            uint8_t buffer[RSA_KEY_SIZE];
            for (uint32_t i = 0; i < recvRSANum; i++) {
                memset(buffer, 0, RSA_KEY_SIZE);
                BN_bin2bn(recvBuf.dataBuffer + i * RSA_KEY_SIZE, RSA_KEY_SIZE, result_);
                // result = hash^d
                BN_mod_exp(result_, result_, serverKeyD_, serverKeyN_, bnCTX_);
                BN_bn2bin(result_, buffer + (RSA_KEY_SIZE - BN_num_bytes(result_)));
                memcpy(recvBuf.dataBuffer + i * RSA_KEY_SIZE, buffer, RSA_KEY_SIZE);
            }
            
            // send the key gen result back to the client
            recvBuf.header->dataSize = recvRSANum * RSA_KEY_SIZE;
            recvBuf.header->currentItemNum = recvRSANum;
            recvBuf.header->messageType = KEY_MANAGER_KEY_GEN_REPLY;
            if (!keyManagerChannel_->SendData(keyClientSSL, recvBuf.sendBuffer,
                sizeof(NetworkHead_t) + recvBuf.header->dataSize)) {
                tool::Logging(myName_.c_str(), "send the key gen errors.\n");
                exit(EXIT_FAILURE);
            }

            gettimeofday(&eProcTime, NULL);
            totalProcessTime += tool::GetTimeDiff(sProcTime, eProcTime);
            _totalKeyGenNum += recvRSANum;
        }
    }

    free(recvBuf.sendBuffer);
    tool::Logging(myName_.c_str(), "thread exits for %s, ID: %u, total process time: %lf\n", 
        clientIP.c_str(), clientID, totalProcessTime);
    return ;
}