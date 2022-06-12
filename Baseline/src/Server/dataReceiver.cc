/**
 * @file dataReceiver.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of data receivers 
 * @version 0.1
 * @date 2021-01-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/dataReceiver.h"

/**
 * @brief Construct a new DataReceiver object
 * 
 * @param absIndexObj the pointer to the index obj
 * @param serverChannel the pointer to the security channel
 */
DataReceiver::DataReceiver(AbsIndex* absIndexObj, SSLConnection* serverChannel) {
    // set up the connection and interface
    serverChannel_ = serverChannel;
    absIndexObj_ = absIndexObj;
    tool::Logging(myName_.c_str(), "init the DataReceiver.\n");
}

/**
 * @brief Destroy the DataReceiver object
 * 
 */
DataReceiver::~DataReceiver() {
    fprintf(stderr, "========DataReceiver Info========\n");
    fprintf(stderr, "total recv batch num: %lu\n", batchNum_);
    fprintf(stderr, "total recv recipe end num: %lu\n", recipeEndNum_);
    fprintf(stderr, "=================================\n");
}

/**
 * @brief the main process to handle new client upload-request connection
 * 
 * @param curClient the ptr to the current client
 * @param enclaveInfo the ptr to the enclave info
 */
void DataReceiver::Run(ClientVar* curClient, EnclaveInfo_t* enclaveInfo) {
    uint32_t recvSize = 0;
    string clientIP;
    SendMsgBuffer_t* recvChunkBuf = &curClient->_recvChunkBuf;
    Container_t* curContainer = &curClient->_curContainer;
    SSL* clientSSL = curClient->_clientSSL;

    struct timeval sProcTime;
    struct timeval eProcTime;
    double totalProcessTime = 0;

    tool::Logging(myName_.c_str(), "the main thread is running.\n");
    while (true) {
        // receive data
        if (!serverChannel_->ReceiveData(clientSSL, recvChunkBuf->sendBuffer, 
            recvSize)) {
            tool::Logging(myName_.c_str(), "client closed socket connect, thread exit now.\n");
            serverChannel_->GetClientIp(clientIP, clientSSL);
            serverChannel_->ClearAcceptedClientSd(clientSSL);
            break;
        } else {
            gettimeofday(&sProcTime, NULL);
            switch (recvChunkBuf->header->messageType) {
                case CLIENT_UPLOAD_CHUNK: {
                    absIndexObj_->ProcessOneBatch(recvChunkBuf, curClient);
                    batchNum_++;
                    break;
                }
                case CLIENT_UPLOAD_RECIPE: {
                    this->ProcessRecipes(curClient);
                    recipeBatchNum_++;
                    break;
                }
                case CLIENT_UPLOAD_RECIPE_END: {
                    // this is the end of one upload
                    absIndexObj_->ProcessTailBatch(curClient);
                    // finalize the file recipe
                    storageCoreObj_->FinalizeRecipe((FileRecipeHead_t*)recvChunkBuf->dataBuffer, 
                        curClient->_recipeWriteHandler);
                    recipeEndNum_++;

                    // update the upload data size
                    FileRecipeHead_t* tmpRecipeHead = (FileRecipeHead_t*)recvChunkBuf->dataBuffer;
                    curClient->_uploadDataSize = tmpRecipeHead->fileSize;
                    break;
                }
                default: {
                    // receive teh wrong message type
                    tool::Logging(myName_.c_str(), "wrong received message type.\n");
                    exit(EXIT_FAILURE);
                }
            }
            gettimeofday(&eProcTime, NULL);
            totalProcessTime += tool::GetTimeDiff(sProcTime, eProcTime);
        }
    }

    // process the last container
    if (curContainer->currentSize != 0) {
        Ocall_WriteContainer(curClient);
    }
    curClient->_inputMQ->done_ = true;
    tool::Logging(myName_.c_str(), "thread exit for %s, ID: %u, total process time: %lf\n", 
        clientIP.c_str(), curClient->_clientID, totalProcessTime);

    enclaveInfo->enclaveProcessTime = totalProcessTime;
    enclaveInfo->logicalDataSize = absIndexObj_->_logicalDataSize;
    enclaveInfo->logicalChunkNum = absIndexObj_->_logicalChunkNum;
    enclaveInfo->uniqueDataSize = absIndexObj_->_uniqueDataSize;
    enclaveInfo->uniqueChunkNum = absIndexObj_->_uniqueChunkNum;
    enclaveInfo->compressedSize = absIndexObj_->_compressedDataSize;
    return ; 
}

/**
 * @brief process a batch of recipes
 * 
 * @param curClient the client var
 */
void DataReceiver::ProcessRecipes(ClientVar* curClient) {
    SendMsgBuffer_t* recvChunkBuf = &curClient->_recvChunkBuf;
    curClient->_keyRecipeWriteHandler.write((char*)recvChunkBuf->dataBuffer,
        recvChunkBuf->header->dataSize);
    return ;
}