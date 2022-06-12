/**
 * @file recvDecoder.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the recv decoder
 * @version 0.1
 * @date 2021-09-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/recvDecoder.h"

/**
 * @brief Construct a new Recv Decoder object
 * 
 * @param serverChannel the ssl connection pointer
 */
RecvDecoder::RecvDecoder(SSLConnection* serverChannel) : AbsRecvDecoder(serverChannel) {
    tool::Logging(myName_.c_str(), "init the RecvDecoder.\n");
}

/**
 * @brief Destroy the Recv Decoder object
 * 
 */
RecvDecoder::~RecvDecoder() {
    fprintf(stderr, "========RecvDecoder Info========\n");
    fprintf(stderr, "read container from file num: %lu\n", readFromContainerFileNum_);
    fprintf(stderr, "=======================================\n");
}

/**
 * @brief the main process
 * 
 * @param curClient the current client ptr
 */
void RecvDecoder::Run(ClientVar* curClient) {
    tool::Logging(myName_.c_str(), "the main thread is running.\n");
    SSL* clientSSL = curClient->_clientSSL;

    uint8_t* readRecipeBuf = curClient->_readRecipeBuf;
    SendMsgBuffer_t* sendChunkBuf = &curClient->_sendChunkBuf;
    uint32_t recvSize = 0;

    if (!serverChannel_->ReceiveData(clientSSL, sendChunkBuf->sendBuffer,
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
        curClient->_recipeReadHandler.read((char*)readRecipeBuf, 
            sizeof(RecipeEntry_t) * sendRecipeBatchSize_);
        size_t readCnt = curClient->_recipeReadHandler.gcount();
        end = curClient->_recipeReadHandler.eof();
        size_t recipeEntryNum = readCnt / sizeof(RecipeEntry_t);
        if (readCnt == 0) {
            break;
        }

        totalRestoreRecipeNum_ += recipeEntryNum;
        this->ProcessRecipeBatch(readRecipeBuf, recipeEntryNum, curClient);
    }

    this->ProcessRecipeTailBatch(curClient);

    // wait the close connection request from the client
    string clientIP;
    if (!serverChannel_->ReceiveData(clientSSL, sendChunkBuf->sendBuffer, 
        recvSize)) {
        tool::Logging(myName_.c_str(), "client closed socket connect, thread exit now.\n");
        serverChannel_->GetClientIp(clientIP, clientSSL);
        serverChannel_->ClearAcceptedClientSd(clientSSL);
    }
    gettimeofday(&eProcTime, NULL);

    tool::Logging(myName_.c_str(), "all job done for %s, thread exits now.\n", clientIP.c_str());
    totalProcTime += tool::GetTimeDiff(sProcTime, eProcTime);

    return ;
}

/**
 * @brief recover a chunk
 * 
 * @param chunkBuffer the chunk buffer
 * @param chunkSize the chunk size
 * @param restoreChunkBuf the restore chunk buffer
 */
void RecvDecoder::RecoverOneChunk(uint8_t* chunkBuffer, uint32_t chunkSize,
    SendMsgBuffer_t* restoreChunkBuf) {
    uint8_t* outputBuffer = restoreChunkBuf->dataBuffer + 
        restoreChunkBuf->header->dataSize;
    
    // try to decompress the chunk
    int decompressedSize = LZ4_decompress_safe((char*)chunkBuffer, 
        (char*)(outputBuffer + sizeof(uint32_t)), chunkSize, MAX_CHUNK_SIZE);
    if (decompressedSize > 0) {
        // it can do the decompression, write back the decompressed chunk size
        memcpy(outputBuffer, &decompressedSize, sizeof(uint32_t));
        restoreChunkBuf->header->dataSize += sizeof(uint32_t) + decompressedSize; 
    } else {
        // it cannot do the decompression
        memcpy(outputBuffer, &chunkSize, sizeof(uint32_t));
        memcpy(outputBuffer + sizeof(uint32_t), chunkBuffer, chunkSize);
        restoreChunkBuf->header->dataSize += sizeof(uint32_t) + chunkSize;
    }

    restoreChunkBuf->header->currentItemNum++;
    return ;
}

/**
 * @brief process a batch of recipe
 * 
 * @param recipeBuffer the read recipe buffer
 * @param recipeNum the read recipe num
 * @param curClient the current client var
 */
void RecvDecoder::ProcessRecipeBatch(uint8_t* recipeBuffer, size_t recipeNum, 
    ClientVar* curClient) {
    ReqContainer_t* reqContainer = (ReqContainer_t*)&curClient->_reqContainer;
    uint8_t* idBuffer = reqContainer->idBuffer;
    uint8_t** containerArray = reqContainer->containerArray;
    SendMsgBuffer_t* sendChunkBuf = &curClient->_sendChunkBuf;

    string tmpContainerIDStr;
    unordered_map<string, uint32_t> tmpContainerMap;
    tmpContainerMap.reserve(CONTAINER_CAPPING_VALUE);
    EnclaveRecipeEntry_t tmpEnclaveRecipeEntry;

    RecipeEntry_t* tmpRecipeEntry;
    tmpRecipeEntry = (RecipeEntry_t*)recipeBuffer;

    for (size_t i = 0; i < recipeNum; i++) {
        // parse the recipe entry one-by-one
        tmpContainerIDStr.assign((char*)tmpRecipeEntry->containerName, CONTAINER_ID_LENGTH);
        tmpEnclaveRecipeEntry.offset = tmpRecipeEntry->offset;
        tmpEnclaveRecipeEntry.length = tmpRecipeEntry->length;

        auto findResult = tmpContainerMap.find(tmpContainerIDStr);
        if (findResult == tmpContainerMap.end()) {
            // this is a unique container entry, it does not exist in current local index
            tmpEnclaveRecipeEntry.containerID = reqContainer->idNum;
            tmpContainerMap[tmpContainerIDStr] = reqContainer->idNum;
            memcpy(idBuffer + reqContainer->idNum * CONTAINER_ID_LENGTH, 
                tmpContainerIDStr.c_str(), CONTAINER_ID_LENGTH);
            reqContainer->idNum++;
        } else {
            // this is a duplicate container entry, using existing result.
            tmpEnclaveRecipeEntry.containerID = findResult->second;
        }
        curClient->_enclaveRecipeBuffer.push_back(tmpEnclaveRecipeEntry);

        // judge whether reach the capping value 
        if (reqContainer->idNum == CONTAINER_CAPPING_VALUE) {
            // start to let outside application to fetch the container data
            this->GetReqContainers(curClient);

            // read chunk from the encrypted container buffer, 
            // write the chunk to the outside buffer
            for (size_t idx = 0; idx < curClient->_enclaveRecipeBuffer.size(); idx++) {
                uint32_t containerID = curClient->_enclaveRecipeBuffer[idx].containerID;
                uint32_t offset = curClient->_enclaveRecipeBuffer[idx].offset;
                uint32_t chunkSize = curClient->_enclaveRecipeBuffer[idx].length;
                uint8_t* chunkBuffer = containerArray[containerID] + offset;
                this->RecoverOneChunk(chunkBuffer, chunkSize, sendChunkBuf);
                if (sendChunkBuf->header->currentItemNum % sendChunkBatchSize_ == 0) {
                    
                    // copy the header to the send buffer
                    sendChunkBuf->header->messageType = SERVER_RESTORE_CHUNK;
                    this->SendBatchChunks(sendChunkBuf, curClient->_clientSSL);

                    sendChunkBuf->header->dataSize = 0;
                    sendChunkBuf->header->currentItemNum = 0;
                }
            }

            // reset
            reqContainer->idNum = 0;
            tmpContainerMap.clear();
            curClient->_enclaveRecipeBuffer.clear(); 
        }
        tmpRecipeEntry++;
    }
    return ;
}

/**
 * @brief process the tail batch of the recipe
 * 
 * @param curClient the current client var
 */
void RecvDecoder::ProcessRecipeTailBatch(ClientVar* curClient) {
    ReqContainer_t* reqContainer = &curClient->_reqContainer;
    uint8_t** containerArray = reqContainer->containerArray;
    SendMsgBuffer_t* sendChunkBuf = &curClient->_sendChunkBuf;

    if (curClient->_enclaveRecipeBuffer.size() != 0) {
        // start to let outside application to fetch the container data
        this->GetReqContainers(curClient);

        uint32_t remainChunkNum = curClient->_enclaveRecipeBuffer.size();
        bool endFlag = 0;
        for (size_t idx = 0; idx < curClient->_enclaveRecipeBuffer.size(); idx++) {
            uint32_t containerID = curClient->_enclaveRecipeBuffer[idx].containerID;
            uint32_t offset = curClient->_enclaveRecipeBuffer[idx].offset;
            uint32_t chunkSize = curClient->_enclaveRecipeBuffer[idx].length;
            uint8_t* chunkBuffer = containerArray[containerID] + offset;
            this->RecoverOneChunk(chunkBuffer, chunkSize, sendChunkBuf);
            remainChunkNum--;
            if (remainChunkNum == 0) {
                // this is the last batch of chunks;
                endFlag = 1;
            }
            if ((sendChunkBuf->header->currentItemNum % sendChunkBatchSize_ == 0) || endFlag) {
                // copy the header to the send buffer
                if (endFlag == 1) {
                    sendChunkBuf->header->messageType = SERVER_RESTORE_FINAL;
                } else {
                    sendChunkBuf->header->messageType = SERVER_RESTORE_CHUNK;
                }
                this->SendBatchChunks(sendChunkBuf, curClient->_clientSSL);

                sendChunkBuf->header->dataSize = 0;
                sendChunkBuf->header->currentItemNum = 0;
            }
        }
    }
    return ;
}

/**
 * @brief Get the Required Containers object 
 * 
 * @param curClient the current client ptr
 */
void RecvDecoder::GetReqContainers(ClientVar* curClient) {
    ReqContainer_t* reqContainer = &curClient->_reqContainer;
    uint8_t* idBuffer = reqContainer->idBuffer; 
    uint8_t** containerArray = reqContainer->containerArray;
    ReadCache* containerCache = curClient->_containerCache;
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
void RecvDecoder::SendBatchChunks(SendMsgBuffer_t* sendChunkBuf, 
    SSL* clientSSL) {
    if (!serverChannel_->SendData(clientSSL, sendChunkBuf->sendBuffer, 
        sizeof(NetworkHead_t) + sendChunkBuf->header->dataSize)) {
        tool::Logging(myName_.c_str(), "send the batch of restored chunks error.\n");
        exit(EXIT_FAILURE);
    }
    return ;
}