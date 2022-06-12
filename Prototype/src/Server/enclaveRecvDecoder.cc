/**
 * @file enclaveRecvDecoder.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of enclave-based recv decoder
 * @version 0.1
 * @date 2021-02-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/enclaveRecvDecoder.h"

/**
 * @brief Construct a new Enclave Recv Decoder object
 * 
 * @param fileName2metaDB the file recipe index
 * @param sslConnection the ssl connection pointer
 * @param eidSGX the id to the enclave
 */
EnclaveRecvDecoder::EnclaveRecvDecoder(SSLConnection* sslConnection,
    sgx_enclave_id_t eidSGX) : AbsRecvDecoder(sslConnection) {
    eidSGX_ = eidSGX;
    Ecall_Init_Restore(eidSGX_);
    tool::Logging(myName_.c_str(), "init the EnclaveRecvDecoder.\n");
}

/**
 * @brief Destroy the Enclave Recv Decoder object
 * 
 */
EnclaveRecvDecoder::~EnclaveRecvDecoder() {
    fprintf(stderr, "========EnclaveRecvDecoder Info========\n");
    fprintf(stderr, "read container from file num: %lu\n", readFromContainerFileNum_);
    fprintf(stderr, "=======================================\n");
}

/**
 * @brief the main process
 * 
 * @param outClient the out-enclave client ptr
 */
void EnclaveRecvDecoder::Run(ClientVar* outClient) {
    tool::Logging(myName_.c_str(), "the main thread is running.\n");
    SSL* clientSSL = outClient->_clientSSL;
    ResOutSGX_t* resOutSGX = &outClient->_resOutSGX;

    uint8_t* readRecipeBuf = outClient->_readRecipeBuf;
    SendMsgBuffer_t* sendChunkBuf = &outClient->_sendChunkBuf;
    uint32_t recvSize = 0;

    if (!dataSecureChannel_->ReceiveData(clientSSL, sendChunkBuf->sendBuffer,
        recvSize)) {
        tool::Logging(myName_.c_str(), "recv the client ready error.\n");
        exit(EXIT_FAILURE);
    } else {
        if (sendChunkBuf->header->messageType != CLIENT_RESTORE_READY) {
            tool::Logging(myName_.c_str(), "wrong type of client ready reply.\n");
            exit(EXIT_FAILURE);
        }
    }

    struct timeval sProcTime;
    struct timeval eProcTime;
    double totalProcTime = 0;
    tool::Logging(myName_.c_str(), "start to read the file recipe.\n");
    gettimeofday(&sProcTime, NULL);
    bool end = false;
    while (!end) {
        // read a batch of the recipe entries from the recipe file
        outClient->_recipeReadHandler.read((char*)readRecipeBuf, 
            sizeof(RecipeEntry_t) * sendRecipeBatchSize_);
        size_t readCnt = outClient->_recipeReadHandler.gcount();
        end = outClient->_recipeReadHandler.eof();
        size_t recipeEntryNum = readCnt / sizeof(RecipeEntry_t);
        if (readCnt == 0) {
            break;
        }

        totalRestoreRecipeNum_ += recipeEntryNum;
        Ecall_ProcRecipeBatch(eidSGX_, readRecipeBuf, recipeEntryNum, 
            resOutSGX);
    }

    Ecall_ProcRecipeTailBatch(eidSGX_, resOutSGX);

    // wait the close connection request from the client
    string clientIP;
    if (!dataSecureChannel_->ReceiveData(clientSSL, sendChunkBuf->sendBuffer, 
        recvSize)) {
        tool::Logging(myName_.c_str(), "client closed socket connect, thread exit now.\n");
        dataSecureChannel_->GetClientIp(clientIP, clientSSL);
        dataSecureChannel_->ClearAcceptedClientSd(clientSSL);
    }
    gettimeofday(&eProcTime, NULL);

    tool::Logging(myName_.c_str(), "all job done for %s, thread exits now.\n", clientIP.c_str());
    totalProcTime += tool::GetTimeDiff(sProcTime, eProcTime);

    return ;
}

/**
 * @brief Get the Required Containers object 
 * 
 * @param outClient the out-enclave client ptr
 */
void EnclaveRecvDecoder::GetReqContainers(ClientVar* outClient) {
    ReqContainer_t* reqContainer = &outClient->_reqContainer;
    uint8_t* idBuffer = reqContainer->idBuffer; 
    uint8_t** containerArray = reqContainer->containerArray;
    ReadCache* containerCache = outClient->_containerCache;
    uint32_t idNum = reqContainer->idNum; 

    // retrieve each container
    string containerNameStr;
    for (size_t i = 0; i < idNum; i++) {
        containerNameStr.assign((char*) (idBuffer + i * CONTAINER_ID_LENGTH), 
            CONTAINER_ID_LENGTH);
        // step-1: check the container cache
        bool cacheHitStatus = containerCache->ExistsInCache(containerNameStr);
        if (cacheHitStatus) {
            // step-2: exist in the container cache, read from the cache, directly copy the data from the cache
            memcpy(containerArray[i], containerCache->ReadFromCache(containerNameStr), 
                MAX_CONTAINER_SIZE);
            continue ;
        } 

        // step-3: not exist in the contain cache, read from disk
        ifstream containerIn;
        string readFileNameStr = containerNamePrefix_ + containerNameStr + containerNameTail_;
        containerIn.open(readFileNameStr, ifstream::in | ifstream::binary);

        if (!containerIn.is_open()) {
            tool::Logging(myName_.c_str(), "cannot open the container: %s\n", readFileNameStr.c_str());
            exit(EXIT_FAILURE);
        }

        // get the data section size (total chunk size - metadata section)
        containerIn.seekg(0, ios_base::end);
        int readSize = containerIn.tellg();
        containerIn.seekg(0, ios_base::beg);

        // read the metadata section
        int containerSize = 0;
        containerSize = readSize;
        // read compression data
        containerIn.read((char*)containerArray[i], containerSize);

        if (containerIn.gcount() != containerSize) {
            tool::Logging(myName_.c_str(), "read size %lu cannot match expected size: %d for container %s.\n",
                containerIn.gcount(), containerSize, readFileNameStr.c_str());
            exit(EXIT_FAILURE);
        } 

        // close the container file
        containerIn.close();
        readFromContainerFileNum_++;
        containerCache->InsertToCache(containerNameStr, containerArray[i], containerSize);
    }
    return ;
}

/**
 * @brief send the restore chunk to the client
 * 
 * @param sendChunkBuf the send chunk buffer
 * @param clientSSL the ssl connection
 */
void EnclaveRecvDecoder::SendBatchChunks(SendMsgBuffer_t* sendChunkBuf, 
    SSL* clientSSL) {
    if (!dataSecureChannel_->SendData(clientSSL, sendChunkBuf->sendBuffer, 
        sizeof(NetworkHead_t) + sendChunkBuf->header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the batch of restored chunks error.\n");
        exit(EXIT_FAILURE);
    }
    return ;
}