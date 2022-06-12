/**
 * @file tedKM.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of the ted key manager
 * @version 0.1
 * @date 2021-12-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/tedKM.h"

/**
 * @brief Construct a new TEDKeyManager object
 * 
 * @param keyManagerChannel the channel for key generation 
 */
TEDKeyManager::TEDKeyManager(SSLConnection* keyManagerChannel) : 
    AbsKeyManager(keyManagerChannel) {
    sketchTable_ = (uint32_t**) malloc(SKETCH_DEPTH * sizeof(uint32_t*));
    for (int i = 0; i < SKETCH_DEPTH; i++) {
        sketchTable_[i] = (uint32_t*) malloc(SKETCH_WIDTH * sizeof(uint32_t));
        memset(sketchTable_[i], 0, SKETCH_WIDTH * sizeof(uint32_t));
    }
    gen_ = mt19937_64(rd_());
    tool::Logging(myName_.c_str(), "init the TED-KeyManager.\n");
}

/**
 * @brief Destroy the TEDKeyManager object
 * 
 */
TEDKeyManager::~TEDKeyManager() {
    for (int i = 0; i < SKETCH_DEPTH; i++) {
        free(sketchTable_[i]);
    }
    free(sketchTable_);
    fprintf(stderr,"========TEDKeyManager Info========\n");
    fprintf(stderr,"key gen num: %lu\n", _totalKeyGenNum);
    fprintf(stderr,"==================================\n");
}

/**
 * @brief the main thread
 * 
 * @param keyClientSSL the client ssl
 */
void TEDKeyManager::Run(SSL* keyClientSSL) {
    uint32_t recvSize = 0;
    string clientIP;
    SendMsgBuffer_t recvBuf;
    recvBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendChunkBatchSize_ * CHUNK_HASH_SIZE);
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
            for (uint32_t i = 0; i < SKETCH_DEPTH; i++) {
                for (uint32_t j = 0; j < SKETCH_WIDTH; j++) {
                    sketchTable_[i][j] = 0;
                }
            }
            break;
        } else {
            gettimeofday(&sProcTime, NULL);
            if (recvBuf.header->messageType != CLIENT_KEY_GEN) {
                tool::Logging(myName_.c_str(), "wrong key gen req type.\n");
                exit(EXIT_FAILURE);
            }

            // perform frequency counting
            uint32_t recvShortHashNum = recvBuf.header->currentItemNum;
            uint8_t tmpHashBuf[SHORT_HASH_SIZE + sizeof(uint32_t) + CHUNK_HASH_SIZE] = {0};
            for (uint32_t i = 0; i < recvShortHashNum; i++) {
                // update the sketch
                uint32_t curFreq = UINT32_MAX;
                uint32_t* shortHash = (uint32_t*) (recvBuf.dataBuffer + i * CHUNK_HASH_SIZE);
                for (uint32_t j = 0; j < SKETCH_DEPTH; j++) {
                    sketchTable_[j][shortHash[j] % SKETCH_WIDTH]++;
                    if (curFreq > sketchTable_[j][shortHash[j] % SKETCH_WIDTH]) {
                        curFreq = sketchTable_[j][shortHash[j] % SKETCH_WIDTH];
                    }
                }

                uint32_t param = floor(curFreq / t_);
                uniform_int_distribution<> dis(0, param);
                param = dis(gen_);
                memcpy(tmpHashBuf, recvBuf.dataBuffer + i * CHUNK_HASH_SIZE, SHORT_HASH_SIZE);
                memcpy(tmpHashBuf + SHORT_HASH_SIZE, &param, sizeof(uint32_t));
                memcpy(tmpHashBuf + SHORT_HASH_SIZE + sizeof(uint32_t), 
                    globalSecret_, CHUNK_HASH_SIZE);
                cryptoObj_->GenerateHash(mdCTX_, tmpHashBuf, SHORT_HASH_SIZE + sizeof(uint32_t) + CHUNK_HASH_SIZE, 
                    recvBuf.dataBuffer + i * CHUNK_HASH_SIZE);
            }

            // send the key gen result back to the client
            recvBuf.header->dataSize = recvShortHashNum * CHUNK_HASH_SIZE;
            recvBuf.header->currentItemNum = recvShortHashNum;
            recvBuf.header->messageType = KEY_MANAGER_KEY_GEN_REPLY;
            if (!keyManagerChannel_->SendData(keyClientSSL, recvBuf.sendBuffer, 
                sizeof(NetworkHead_t) + recvBuf.header->dataSize)) {
                tool::Logging(myName_.c_str(), "send the key gen errors.\n");
                exit(EXIT_FAILURE);
            }

            gettimeofday(&eProcTime, NULL);
            totalProcessTime += tool::GetTimeDiff(sProcTime, eProcTime);
            _totalKeyGenNum += recvShortHashNum;
        }
    }

    free(recvBuf.sendBuffer);
    tool::Logging(myName_.c_str(), "thread exits for %s, ID: %u, total process time: %lf\n", 
        clientIP.c_str(), clientID, totalProcessTime);
    return ;
}