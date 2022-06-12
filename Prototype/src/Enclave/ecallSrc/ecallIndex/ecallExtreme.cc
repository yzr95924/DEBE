/**
 * @file ecallExtreme.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in the extreme index inside the enclave
 * @version 0.1
 * @date 2020-12-30
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/ecallExtreme.h"

/**
 * @brief Construct a new Ecall Extreme Index object
 * 
 */
EcallExtremeBinIndex::EcallExtremeBinIndex() {
    primaryIndex_ = new unordered_map<string, string>();
    if (ENABLE_SEALING) {
        if (!this->LoadPrimaryIndex()) {
            Enclave::Logging(myName_.c_str(), "do not need to load the primary index.\n");
        }
    }
    Enclave::Logging(myName_.c_str(), "init the EcallExtremeBinIndex.\n");
}

/**
 * @brief Destroy the Ecall Extreme Index object
 * 
 */
EcallExtremeBinIndex::~EcallExtremeBinIndex() {
    if (ENABLE_SEALING) {
        if (primaryIndex_->size() != 0) {
            this->PersistPrimaryIndex();
        }
    }

    delete primaryIndex_;
    Enclave::Logging(myName_.c_str(), "========EcallExtremeBinIndex Info========\n");
    Enclave::Logging(myName_.c_str(), "logical chunk num: %lu\n", _logicalChunkNum);
    Enclave::Logging(myName_.c_str(), "logical data size: %lu\n", _logicalDataSize);
    Enclave::Logging(myName_.c_str(), "unique chunk num: %lu\n", _uniqueChunkNum);
    Enclave::Logging(myName_.c_str(), "unique data size: %lu\n", _uniqueDataSize);
    Enclave::Logging(myName_.c_str(), "compressed data size: %lu\n", _compressedDataSize);
    Enclave::Logging(myName_.c_str(), "total segment num: %lu\n", totalSegmentNum_);
    Enclave::Logging(myName_.c_str(), "unique segment num: %lu\n", uniqueSegmentNum_);
    Enclave::Logging(myName_.c_str(), "different segment num: %lu\n", differentSegmentNum_);
    Enclave::Logging(myName_.c_str(), "duplicate segment num: %lu\n", duplicateSegmentNum_);
    Enclave::Logging(myName_.c_str(), "==========================================\n");
}

/**
 * @brief process one batch
 * 
 * @param recvChunkBuf the recv chunk buffer
 * @param upOutSGX the pointer to enclave-related var 
 */
void EcallExtremeBinIndex::ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, 
    UpOutSGX_t* upOutSGX) {
    // the in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;
    EVP_MD_CTX* mdCtx = sgxClient->_mdCtx;
    uint8_t* recvBuffer = sgxClient->_recvBuffer;
    uint8_t* sessionKey = sgxClient->_sessionKey;
    Segment_t* segment = &sgxClient->_segment;

    // decrypt the received data with the session key
    cryptoObj_->SessionKeyDec(cipherCtx, recvChunkBuf->dataBuffer,
        recvChunkBuf->header->dataSize, sessionKey, recvBuffer);   
    
    // get the chunk num
    uint32_t chunkNum = recvChunkBuf->header->currentItemNum;

    // start to process each chunk
    uint32_t tmpChunkSize = 0;
    string tmpHashStr;
    tmpHashStr.resize(CHUNK_HASH_SIZE, 0);
    size_t offset = 0;
    uint32_t chunkHashVal = 0;
    SegmentMeta_t* curSegmentMetaPtr;
    
    for (size_t i = 0; i < chunkNum; i++) {
        // step-2: compute the hash over the plaintext chunk
        // read the chunk size
        memcpy(&tmpChunkSize, recvBuffer + offset, sizeof(tmpChunkSize));
        offset += sizeof(tmpChunkSize);

        cryptoObj_->GenerateHash(mdCtx, recvBuffer + offset, 
            tmpChunkSize, (uint8_t*)&tmpHashStr[0]);

        chunkHashVal = this->ConvertHashToValue((uint8_t*)&tmpHashStr[0]);

        if (this->IsEndOfSegment(chunkHashVal, tmpChunkSize, segment)) {
            // this is the end of the segment
            this->ProcessOneSegment(segment, upOutSGX);
            this->ResetCurrentSegment(sgxClient);
        } 

        // add this chunk to current segment buffer        
        memcpy(segment->buffer + segment->segmentSize,
            recvBuffer + offset, tmpChunkSize);
        curSegmentMetaPtr = segment->metadata + 
            segment->chunkNum;
        memcpy(curSegmentMetaPtr->chunkHash, &tmpHashStr[0], CHUNK_HASH_SIZE);
        curSegmentMetaPtr->chunkSize = tmpChunkSize;

        // update the corresponding metadata
        segment->segmentSize += tmpChunkSize;
        segment->chunkNum++;
        if (chunkHashVal < segment->minHashVal) {
            segment->minHashVal = chunkHashVal;
            memcpy(segment->minHash, &tmpHashStr[0], CHUNK_HASH_SIZE);
        }

        // update the recv buffer offset
        offset += tmpChunkSize;

        // update the statistic
        _logicalDataSize += tmpChunkSize;
        _logicalChunkNum++;
    }

    // reset
    memset(recvBuffer, 0, Enclave::sendChunkBatchSize_ * sizeof(Chunk_t));
    memset(recvChunkBuf->dataBuffer, 0, Enclave::sendChunkBatchSize_ * sizeof(Chunk_t));
    return ;
}

/**
 * @brief start to process one batch
 * 
 * @param segment the pointer to current segment
 * @param upOutSGX the pointer to enclave var 
 */
void EcallExtremeBinIndex::ProcessOneSegment(Segment_t* segment, 
    UpOutSGX_t* upOutSGX) {
    // for in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient; 
    EVP_MD_CTX* mdCtx = sgxClient->_mdCtx;
    Recipe_t* inRecipe = &sgxClient->_inRecipe;
    
    // start to process the segment
    string tmpSegmentHash;
    tmpSegmentHash.resize(CHUNK_HASH_SIZE, 0);
    string tmpSegmentMinHash;
    tmpSegmentMinHash.resize(CHUNK_HASH_SIZE, 0);
    string binPointerStr;
    binPointerStr.resize(SEGMENT_ID_LENGTH, 0);
    unordered_map<string, string> tmpDedupIndex;

    // compute the hash of the segment
    uint32_t chunkNum = segment->chunkNum;
    cryptoObj_->GenerateHash(mdCtx, (uint8_t*)segment->metadata, 
        chunkNum * sizeof(SegmentMeta_t), (uint8_t*)&tmpSegmentHash[0]);

    // find the min-hash of the segment
    tmpSegmentMinHash.assign((char*)segment->minHash, CHUNK_HASH_SIZE);

    // check the primary index via the min-hash of the segment
    auto findResult = primaryIndex_->find(tmpSegmentMinHash);
    PrimaryValue_t* primaryValuePtr;
    string primaryValueStr;
    primaryValueStr.resize(sizeof(PrimaryValue_t), 0);
    SegmentMeta_t* metaDataPtr = segment->metadata;
    size_t offset = 0;
    string tmpChunkFp;
    tmpChunkFp.resize(CHUNK_HASH_SIZE, 0);
    string tmpChunkAddrStr;
    tmpChunkAddrStr.resize(sizeof(RecipeEntry_t), 0);

    if (findResult != primaryIndex_->end()) {
        // find this min-hash in the primary index, further compare the segment hash
        primaryValuePtr = (PrimaryValue_t*)&findResult->second[0];

        // fetch the bin from the bin store
        binPointerStr.assign((char*)primaryValuePtr->binID_, SEGMENT_ID_LENGTH);
        if (!this->FetchBin(binPointerStr, tmpDedupIndex, upOutSGX)) {
            Ocall_SGX_Exit_Error("EcallExtremeBinIndex: cannot find the bin.");
        }

        // compare this hash with current segment hash
        if (tmpSegmentHash.compare(string((char*)primaryValuePtr->segmentHash, CHUNK_HASH_SIZE)) != 0) {
            // this segment is different from the previous segment
            // conduct deduplication of the current segment
            for (size_t i = 0; i < chunkNum; i++) {
                tmpChunkFp.assign((char*)metaDataPtr->chunkHash, CHUNK_HASH_SIZE);

                auto tmpFindResult = tmpDedupIndex.find(tmpChunkFp);
                if (tmpFindResult != tmpDedupIndex.end()) {
                    // this chunk is duplicate
                    this->UpdateFileRecipe(tmpFindResult->second, inRecipe, upOutSGX);
                } else {
                    // this chunk is unique
                    _uniqueChunkNum++;
                    _uniqueDataSize += metaDataPtr->chunkSize;

                    // process one unique chunk
                    this->ProcessUniqueChunk((RecipeEntry_t*)&tmpChunkAddrStr[0], 
                        segment->buffer + offset, metaDataPtr->chunkSize,
                        upOutSGX);

                    // update the chunk to the tmp deduplication index
                    tmpDedupIndex.insert({tmpChunkFp, tmpChunkAddrStr});
                    this->UpdateFileRecipe(tmpChunkAddrStr, inRecipe, upOutSGX);
                }

                offset += metaDataPtr->chunkSize;
                metaDataPtr++;
            }

            // update the bin store 
            if (!this->UpdateBinStore(binPointerStr, tmpDedupIndex, upOutSGX)) {
                Ocall_SGX_Exit_Error("EcallExtremeBinIndex: cannot update the bin store.");
            }

            differentSegmentNum_++;
        } else {
            // this segment is same as previous segment, directly get the address from the bin store
            for (size_t i = 0; i < chunkNum; i++) {
                // chunk hash
                tmpChunkFp.assign((char*)metaDataPtr->chunkHash, CHUNK_HASH_SIZE);
                auto tmpFindResult = tmpDedupIndex.find(tmpChunkFp);
                if (tmpFindResult != tmpDedupIndex.end()) {
                    // parse the chunk address
                    this->UpdateFileRecipe(tmpFindResult->second, inRecipe, 
                        upOutSGX);
                } else {
                    // error
                    Ocall_SGX_Exit_Error("EcallExtremeBinIndex: cannot find the chunk address "
                        "of tmp deduplication index.");
                }

                metaDataPtr++; 
            }

            // do not need to update the index bin store
            duplicateSegmentNum_++;
        }
    } else {
        // cannot find the min-hash of this segment in the primary index
        for (size_t i = 0; i < chunkNum; i++) {
            // chunk hash
            tmpChunkFp.assign((char*)metaDataPtr->chunkHash, CHUNK_HASH_SIZE);

            auto tmpFindResult = tmpDedupIndex.find(tmpChunkFp);
            if (tmpFindResult != tmpDedupIndex.end()) {
                // this chunk is duplicate 
                this->UpdateFileRecipe(tmpFindResult->second, inRecipe, 
                    upOutSGX);
            } else {
                // this chunk is unique 
                _uniqueDataSize += metaDataPtr->chunkSize;
                _uniqueChunkNum++;

                // process one unique chunk
                this->ProcessUniqueChunk((RecipeEntry_t*)&tmpChunkAddrStr[0],
                    segment->buffer + offset, metaDataPtr->chunkSize,
                    upOutSGX);

                // add this chunk to the tmp deduplication index
                tmpDedupIndex.insert({tmpChunkFp, tmpChunkAddrStr});
                this->UpdateFileRecipe(tmpChunkAddrStr, inRecipe, upOutSGX);
            }

            offset += metaDataPtr->chunkSize;
            metaDataPtr++;
        }

        // update the bin store
        primaryValuePtr = (PrimaryValue_t*)&primaryValueStr[0];
        memcpy(primaryValuePtr->segmentHash, &tmpSegmentHash[0], CHUNK_HASH_SIZE);
        Ocall_CreateUUID(primaryValuePtr->binID_, SEGMENT_ID_LENGTH);
        binPointerStr.assign((char*)primaryValuePtr->binID_, SEGMENT_ID_LENGTH);
        primaryIndex_->insert({tmpSegmentMinHash, primaryValueStr});
    
        // update the bin store here 
        if (!this->UpdateBinStore(binPointerStr, tmpDedupIndex, upOutSGX)) {
            Ocall_SGX_Exit_Error("EcallExtremeBinIndex: cannot update the bin store.");
        }

        uniqueSegmentNum_++;
    }

    totalSegmentNum_++;

    return ;
}

/**
 * @brief fetch the bin value in to the dedup index
 * 
 * @param binAddress the bin address
 * @param tmpDedupIndex the reference to the tmp dedup index
 * @param upOutSGX the pointer to the enclave var
 * @return true successes 
 * @return false fails
 */
bool EcallExtremeBinIndex::FetchBin(string& binAddress, 
    unordered_map<string, string>& tmpDedupIndex, UpOutSGX_t* upOutSGX) {
    // the in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;

    string binValue;
    string cipherBinValue;
    string tmpChunkFp;
    string tmpChunkAddrStr;
    BinValue_t* tmpBinValue;

    // step: query the index store
    tmpChunkFp.resize(CHUNK_HASH_SIZE, 0);
    tmpChunkAddrStr.resize(sizeof(RecipeEntry_t), 0);

    bool status = this->ReadIndexStore(binAddress, cipherBinValue,
        upOutSGX);
    if (status == false) {
        Enclave::Logging(myName_.c_str(), "cannot find the bin from the bin store.\n");
        return false;
    }

    // do decryption
    binValue.resize(cipherBinValue.size(), 0);
    cryptoObj_->IndexAESCMCDec(cipherCtx, (uint8_t*)&cipherBinValue[0], 
        cipherBinValue.size(), Enclave::indexQueryKey_, (uint8_t*)&binValue[0]);

    // step: load those entries into the tmp deduplication index
    size_t chunkNum = binValue.length() / sizeof(BinValue_t);
    for (size_t i = 0; i < chunkNum; i++) {
        // add each chunk to the tmp deduplication index
        tmpBinValue = (BinValue_t*)(&binValue[0] + i * sizeof(BinValue_t));
        tmpChunkFp.assign((const char*)tmpBinValue->chunkFp, CHUNK_HASH_SIZE);
        memcpy(&tmpChunkAddrStr[0], &tmpBinValue->address, sizeof(RecipeEntry_t));
        tmpDedupIndex.insert({tmpChunkFp, tmpChunkAddrStr});
    }

    return true;
}

/**
 * @brief update the bin value to the store
 * 
 * @param binAddress the bin address
 * @param tmpDedupIndex the reference to the tmp dedup index
 * @param upOutSGX the pointer to the enclcave var
 * @return true successes
 * @return false fails
 */
bool EcallExtremeBinIndex::UpdateBinStore(string& binAddress, 
    unordered_map<string, string>& tmpDedupIndex, UpOutSGX_t* upOutSGX) {
    // the in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;

    vector<BinValue_t> binValueBuffer;

    // update the bin store <binAddress, binValue>
    size_t entryNum = tmpDedupIndex.size();
    BinValue_t tmpBinValue;

    // step: serialize the index into the bin value buffer
    for (auto it = tmpDedupIndex.begin(); it != tmpDedupIndex.end(); it++) {
        // <chunk fp, chunk address>
        memcpy(tmpBinValue.chunkFp, it->first.c_str(), CHUNK_HASH_SIZE);
        memcpy(&tmpBinValue.address, it->second.c_str(), sizeof(RecipeEntry_t));
        binValueBuffer.push_back(tmpBinValue);
    }

    // step: encryption the binValue;
    string cipherBinValue;
    cipherBinValue.resize(entryNum * sizeof(BinValue_t), 0);
    cryptoObj_->IndexAESCMCEnc(cipherCtx, (uint8_t*)&binValueBuffer[0],
        entryNum*sizeof(BinValue_t), Enclave::indexQueryKey_, (uint8_t*)&cipherBinValue[0]);

    // step: write the bin value buffer to the index store
    if (!this->UpdateIndexStore(binAddress, (const char*)&cipherBinValue[0], entryNum * sizeof(BinValue_t))) {
        Enclave::Logging(myName_.c_str(), "cannot update the bin store.\n");
        return false;
    }

    return true;
}

/**
 * @brief process the tailed batch when received the end of the recipe flag
 * 
 * @param upOutSGX the pointer to enclave-related var
 */
void EcallExtremeBinIndex::ProcessTailBatch(UpOutSGX_t* upOutSGX) {
    // the in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    Recipe_t* inRecipe = &sgxClient->_inRecipe;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;
    uint8_t* masterKey = sgxClient->_masterKey;
    Segment_t* segment = &sgxClient->_segment;

    // check the tail segment first
    if (segment->chunkNum != 0) {
        this->ProcessOneSegment(segment, upOutSGX);
        this->ResetCurrentSegment(sgxClient);
    }

    if (inRecipe->recipeNum != 0) {
        // the out-enclave info
        Recipe_t* outRecipe = (Recipe_t*)upOutSGX->outRecipe;
        cryptoObj_->EncryptWithKey(cipherCtx, inRecipe->entryList,
            inRecipe->recipeNum * sizeof(RecipeEntry_t), masterKey,
            outRecipe->entryList);
        outRecipe->recipeNum = inRecipe->recipeNum;
        Ocall_UpdateFileRecipe(upOutSGX->outClient);
        inRecipe->recipeNum = 0;
    }

    if (sgxClient->_inContainer.curSize != 0) {
        memcpy(upOutSGX->curContainer->body, sgxClient->_inContainer.buf,
            sgxClient->_inContainer.curSize);
        upOutSGX->curContainer->currentSize = sgxClient->_inContainer.curSize;
    }

    return ;
}

/**
 * @brief read the primary index from the sealed data
 * 
 * @return true success
 * @return false fail
 */
bool EcallExtremeBinIndex::LoadPrimaryIndex() {
    // compute the sealed file size
    size_t sealedDataSize;
    Ocall_InitReadSealedFile(&sealedDataSize, SEALED_PRIMAR_INDEX);
    if (sealedDataSize == 0) {
        return false;
    }

    // prepare the buffer
    uint8_t* sealedDataBuffer = (uint8_t*) malloc(sizeof(uint8_t) * sealedDataSize);
    Enclave::ReadFileToBuffer(sealedDataBuffer, static_cast<size_t>(sealedDataSize),
        SEALED_PRIMAR_INDEX);
    Ocall_CloseReadSealedFile(SEALED_PRIMAR_INDEX);

    // parse the buffer to the index
    size_t entryNum = sealedDataSize / (CHUNK_HASH_SIZE + sizeof(PrimaryValue_t));
    string minHashStr;
    minHashStr.resize(CHUNK_HASH_SIZE, 0);
    string primaryValueStr;
    primaryValueStr.resize(sizeof(PrimaryValue_t), 0);
    for (size_t i = 0; i < entryNum; i++) {
        memcpy(&minHashStr[0], sealedDataBuffer + i * (CHUNK_HASH_SIZE + sizeof(PrimaryValue_t)),
            CHUNK_HASH_SIZE);
        memcpy(&primaryValueStr[0], sealedDataBuffer + i * (CHUNK_HASH_SIZE + sizeof(PrimaryValue_t)) + CHUNK_HASH_SIZE,
            sizeof(PrimaryValue_t));
        primaryIndex_->insert({minHashStr, primaryValueStr});
    }

    free(sealedDataBuffer);
    return true;
}

/**
 * @brief seal the primary index to the disk
 * 
 * @return true success
 * @return false fail
 */
bool EcallExtremeBinIndex::PersistPrimaryIndex() {
    // compute the deduplication index size
    uint32_t primaryIndexSize = primaryIndex_->size() * (CHUNK_HASH_SIZE + sizeof(PrimaryValue_t));

    // prepare the buffer, serialize the index to the buffer
    uint8_t* tmpIndexBuffer = (uint8_t*) malloc(sizeof(uint8_t) * primaryIndexSize);
    size_t offset = 0;
    for (auto it = primaryIndex_->begin(); it != primaryIndex_->end(); it++) {
        memcpy(tmpIndexBuffer + offset * (CHUNK_HASH_SIZE + sizeof(PrimaryValue_t)), 
            it->first.c_str(), CHUNK_HASH_SIZE);
        memcpy(tmpIndexBuffer + offset * (CHUNK_HASH_SIZE + sizeof(PrimaryValue_t)) + CHUNK_HASH_SIZE,
            it->second.c_str(), sizeof(PrimaryValue_t));
        offset++;
    }

    // write the buffer to the file
    bool persistenceStatus;
    Ocall_InitWriteSealedFile(&persistenceStatus, SEALED_PRIMAR_INDEX);
    if (persistenceStatus == false) {
        Ocall_SGX_Exit_Error("EcallExtremeBinIndex: cannot init the sealed file.");
    }
    Enclave::WriteBufferToFile(tmpIndexBuffer, primaryIndexSize, SEALED_PRIMAR_INDEX);
    Ocall_CloseWriteSealedFile(SEALED_PRIMAR_INDEX);

    free(tmpIndexBuffer);
    return true;
}