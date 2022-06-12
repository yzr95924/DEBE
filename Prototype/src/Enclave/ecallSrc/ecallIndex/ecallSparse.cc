/**
 * @file ecallSparseIndex.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces defined in sparse index inside the enclave
 * @version 0.1
 * @date 2021-01-01
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/ecallSparse.h"

/**
 * @brief Construct a new Ecall Sparse Index object
 * 
 * @param enclaveConfig the pointer to the enclave config
 */
EcallSparseIndex::EcallSparseIndex() {
    hookIndex_ = new unordered_map<string, list<string>>();

    Enclave::Logging(myName_.c_str(), "start to load the index.\n");
    if (ENABLE_SEALING) {
        if (!this->LoadHookIndex()) {
            Enclave::Logging(myName_.c_str(), "do not need to load the hook index.\n");
        }
    }
    Enclave::Logging(myName_.c_str(), "init the EcallSparseIndex.\n");
}


/**
 * @brief Destroy the Ecall Sparse Index object
 * 
 */
EcallSparseIndex::~EcallSparseIndex() {
    if (ENABLE_SEALING) {
        if (hookIndex_->size() != 0) {
            this->PersistHookIndex();
        }
    }
    delete hookIndex_;
    Enclave::Logging(myName_.c_str(), "========EcallSparseIndex Info========\n");
    Enclave::Logging(myName_.c_str(), "logical chunk num: %lu\n", _logicalChunkNum);
    Enclave::Logging(myName_.c_str(), "logical data size: %lu\n", _logicalDataSize);
    Enclave::Logging(myName_.c_str(), "unique chunk num: %lu\n", _uniqueChunkNum);
    Enclave::Logging(myName_.c_str(), "unique data size: %lu\n", _uniqueDataSize);
    Enclave::Logging(myName_.c_str(), "compressed data size: %lu\n", _compressedDataSize);
    Enclave::Logging(myName_.c_str(), "total segment num: %lu\n", totalSegmentNum_);
    Enclave::Logging(myName_.c_str(), "=====================================\n");
}

/**
 * @brief process one batch
 * 
 * @param recvChunkBuf the recv chunk buffer
 * @param upOutSGX the pointer to enclave-related var 
 */
void EcallSparseIndex::ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, 
    UpOutSGX_t* upOutSGX) {
    // the in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;
    EVP_MD_CTX* mdCtx = sgxClient->_mdCtx;
    uint8_t* recvBuffer = sgxClient->_recvBuffer;
    uint8_t* sessionKey = sgxClient->_sessionKey;
    Segment_t* segment = &sgxClient->_segment;

    // step-1: decrypt the received data with the session key
    cryptoObj_->SessionKeyDec(cipherCtx, recvChunkBuf->dataBuffer,
        recvChunkBuf->header->dataSize, sessionKey, recvBuffer);

    // get the chunk num
    uint32_t chunkNum = recvChunkBuf->header->currentItemNum;

    // start to processe each chunk
    uint32_t tmpChunkSize = 0;
    string tmpHashStr;
    tmpHashStr.resize(CHUNK_HASH_SIZE, 0);
    size_t offset = 0;
    uint32_t chunkHashVal = 0;
    SegmentMeta_t* curSegmentMetaPtr;

    for (size_t i = 0; i < chunkNum; i++) {
        // compute the hash over the plaintext chunk
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

        // update the recv buffer offset
        offset += tmpChunkSize;

        // update the statistics
        _logicalDataSize += tmpChunkSize;
        _logicalChunkNum++;
    }

    // reset
    memset(recvBuffer, 0, Enclave::sendChunkBatchSize_ * sizeof(Chunk_t));
    memset(recvChunkBuf->dataBuffer, 0, Enclave::sendChunkBatchSize_ * sizeof(Chunk_t));
    return ;
}

/**
 * @brief process the tailed batch when received the end of the recipe flag
 * 
 * @param upOutSGX the pointer to enclave-related var
 */
void EcallSparseIndex::ProcessTailBatch(UpOutSGX_t* upOutSGX) {
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
 * @brief judge whether the chunk is the hook
 * 
 * @param chunkFp chunk fingerprint
 * @return true is the hook of this segment
 * @return false is not the hook of this segment
 */
bool EcallSparseIndex::IsHook(const uint8_t* chunkFp) {
    uint32_t checkedBit = 0;
    uint32_t currentByte = 0;
    uint32_t maskBit = 1;
    while (checkedBit < SPARSE_SAMPLE_RATE) {
        uint8_t tmp = chunkFp[currentByte];
        uint8_t mask = 1 << (8 - maskBit);
        if ((tmp & mask) != 0) {
            return false;
        }
        checkedBit++;
        maskBit++;
        if (maskBit == 9) {
            maskBit = 1;
            currentByte++;
        }
    }
    return true;
}

/**
 * @brief update the manifest candidate table
 * 
 * @param inputSet the manifest set of the given hook
 * @param candidateSegmentList the candidate segment list 
 */
void EcallSparseIndex::UpdateCandidateList(list<string>& inputSet, 
    unordered_map<string, uint32_t>& candidateSegmentList) {
    for (auto it = inputSet.begin(); it != inputSet.end(); it++) {
        auto findResult = candidateSegmentList.find(*it);
        if (findResult != candidateSegmentList.end()) {
            // this manifest is existed in the candidate set
            findResult->second++;
        } else {
            // this manifest is not existed in the candidate list 
            candidateSegmentList.insert({*it, 1});
        }
    }
    return ;
}

// Comparetor function to sort pairs
// according to second value
bool Cmp(pair<string, uint32_t>& a,
        pair<string, uint32_t>& b) {
    return a.second > b.second;
}

/**
 * @brief find the top-k segment, and load the manifest to the deduplication index
 * 
 * @param candidateSegmentList the candidate segment list 
 * @param dedupIndex the deduplication index
 * @param upOutSGX the pointer to the enclave var
 */
void EcallSparseIndex::FindChampionSegment(unordered_map<string, uint32_t>& candidateSegmentList,
    unordered_map<string, string>& dedupIndex, UpOutSGX_t* upOutSGX) {
    size_t totalCandidateSegment = candidateSegmentList.size();
    if (totalCandidateSegment == 0) {
        return ; 
    } else if ((SPARSE_CHAMPION_NUM > totalCandidateSegment) && (totalCandidateSegment > 0)) {
        for (auto it = candidateSegmentList.begin(); it != candidateSegmentList.end(); it++) {
            this->LoadManifiest(it->first, dedupIndex, upOutSGX);
        }
        return ;
    } else {
        // sort the hash map by value, and choose the top-k items
        vector<pair<string, uint32_t>> tmpList;
        for (auto it = candidateSegmentList.begin(); it != candidateSegmentList.end(); it++) {
            tmpList.push_back(*it);
        }

        // find the top-k items
        sort(tmpList.begin(), tmpList.end(), Cmp);

        for (size_t i = 0; i < SPARSE_CHAMPION_NUM; i++) {
            this->LoadManifiest(tmpList[i].first, dedupIndex, upOutSGX);
        }

        tmpList.clear();
        return ;
    }
}

/**
 * @brief update the deduplication index according to the segment list
 * 
 * @param segmentID the segment ID 
 * @param dedupIndex the reference to the deduplication index
 * @param upOutSGX the pointer to the enclave var
 */
void EcallSparseIndex::LoadManifiest(string segmentID,
    unordered_map<string, string>& dedupIndex, UpOutSGX_t* upOutSGX) {
    // the in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;

    string cipherTextManifestValue;
    bool status = this->ReadIndexStore(segmentID, cipherTextManifestValue,
        upOutSGX);
    if (!status) {
        Ocall_SGX_Exit_Error("EcallSparseIndex: cannot find the manifest in the manifest store.");
    }

    // decryption the string
    string manifestValue;
    manifestValue.resize(cipherTextManifestValue.size(), 0);
    cryptoObj_->IndexAESCMCDec(cipherCtx, (uint8_t*)&cipherTextManifestValue[0], 
        cipherTextManifestValue.size(), Enclave::indexQueryKey_, (uint8_t*)&manifestValue[0]);
    // decryption the string

    size_t chunkAddressNum = manifestValue.size() / sizeof(BinValue_t);
    string tmpFp;
    tmpFp.resize(CHUNK_HASH_SIZE, 0);
    string tmpAddress;
    tmpAddress.resize(sizeof(RecipeEntry_t), 0);
    for (size_t i = 0; i < chunkAddressNum; i++) {
        memcpy(&tmpFp[0], manifestValue.c_str() + i * sizeof(BinValue_t), CHUNK_HASH_SIZE);
        memcpy(&tmpAddress[0], manifestValue.c_str() + i * sizeof(BinValue_t) + CHUNK_HASH_SIZE,
            sizeof(RecipeEntry_t));
        dedupIndex.insert({tmpFp, tmpAddress});
    }

    return ;
}

/**
 * @brief update the manifest store according to the manifest value buffer
 * 
 * @param manifestID the ID of the current manifest
 * @param currentBinValueBuffer the reference to the bin value buffer 
 * @param upOutSGX the pointer to the enclave var
 * @return true success
 * @return false fails
 */
bool EcallSparseIndex::UpdateManifiestStore(string& manifestID,
    const vector<BinValue_t>& currentBinValueBuffer, UpOutSGX_t* upOutSGX) {
    // the in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;

    size_t manifestValueSize = currentBinValueBuffer.size() * sizeof(BinValue_t);
    bool status;

    // Encryption the manifestValue;
    string cipherManifestValue;
    cipherManifestValue.resize(manifestValueSize, 0);
    cryptoObj_->IndexAESCMCEnc(cipherCtx, (uint8_t*)&currentBinValueBuffer[0], 
        manifestValueSize, Enclave::indexQueryKey_, (uint8_t*)&cipherManifestValue[0]);
    // end of the encryption

    status = this->UpdateIndexStore(manifestID, (const char*)&cipherManifestValue[0],
        manifestValueSize);
    
    if (!status) {
        Enclave::Logging(myName_.c_str(), "cannot update the manifest store.\n");
        return false;
    }

    return true;
}


/**
 * @brief update the hook index for the given manifestID
 * 
 * @param manifest the ID of current manifest
 * @param currentHookSet the current hook set
 */
void EcallSparseIndex::UpdateHookIndex(string& manifest, set<string>& currentHookSet) {
    for (auto it = currentHookSet.begin(); it != currentHookSet.end(); it++) {
        auto findResult = hookIndex_->find(*it);
        if (findResult != hookIndex_->end()) {
            // it exists in the hook index
            size_t currentListSize = findResult->second.size();
            if (currentListSize < SPARSE_MANIFIEST_CAP_NUM) {
                // less than the cap size
                findResult->second.push_back(manifest);
            } else {
                findResult->second.pop_front();
                findResult->second.push_back(manifest);
            }
        } else {
            // it does not exist in the hook index
            list<string> manifiestList = {manifest};
            hookIndex_->insert({*it, manifiestList});
        }
    }

    return ;
}


 /**
 * @brief persist the hook index into disk
 * 
 * @return true success
 * @return false fail
 */
bool EcallSparseIndex::PersistHookIndex() {
    // step: compute the hook index size
    uint32_t hookIndexSize = 0;
    for (auto it = hookIndex_->begin(); it != hookIndex_->end(); it++) {
        hookIndexSize += CHUNK_HASH_SIZE;
        hookIndexSize += sizeof(size_t);
        size_t manifestListSize = it->second.size() * SEGMENT_ID_LENGTH;
        hookIndexSize += manifestListSize;
    }

    // step: serialize the hook index into the buffer
    uint8_t* tmpIndexBuffer = (uint8_t*) malloc(sizeof(uint8_t) * hookIndexSize);
    size_t offset = 0;
    for (auto it = hookIndex_->begin(); it != hookIndex_->end(); it++) {
        memcpy(tmpIndexBuffer + offset, it->first.c_str(), CHUNK_HASH_SIZE);
        offset += CHUNK_HASH_SIZE;
        size_t manifestListSize = it->second.size();
        memcpy(tmpIndexBuffer + offset, &manifestListSize, sizeof(size_t));
        offset += sizeof(size_t);
        for (auto indexItem = it->second.begin(); indexItem != it->second.end(); indexItem++) {
            memcpy(tmpIndexBuffer + offset, indexItem->c_str(), SEGMENT_ID_LENGTH);
            offset += SEGMENT_ID_LENGTH;
        }
    }

    // step: write the buffer to the file
    bool persistenceStatus;
    Ocall_InitWriteSealedFile(&persistenceStatus, SEALED_SPARSE_INDEX);
    if (persistenceStatus == false) {
        Ocall_SGX_Exit_Error("EcallSparseIndex: cannot init the sealed file.");
    }
    Enclave::WriteBufferToFile(tmpIndexBuffer, hookIndexSize, SEALED_SPARSE_INDEX);
    Ocall_CloseWriteSealedFile(SEALED_SPARSE_INDEX);

    free(tmpIndexBuffer);
    return true;
}


/**
 * @brief read the hook index from sealed data
 * 
 * @return true success
 * @return false fail
 */
bool EcallSparseIndex::LoadHookIndex() {
    // step: compute the sealed file size
    size_t sealedDataSize;
    Ocall_InitReadSealedFile(&sealedDataSize, SEALED_SPARSE_INDEX);
    if (sealedDataSize == 0) {
        return false;
    }

    // step: prepare the buffer
    uint8_t* sealedDataBuffer = (uint8_t*) malloc(sizeof(uint8_t) * sealedDataSize);
    Enclave::ReadFileToBuffer(sealedDataBuffer, static_cast<size_t>(sealedDataSize), SEALED_SPARSE_INDEX);
    Ocall_CloseReadSealedFile(SEALED_SPARSE_INDEX);

    // step: parse the buffer to the index
    size_t offset = 0;
    string tmpChunkFp;
    string tmpManifestID;
    tmpChunkFp.resize(CHUNK_HASH_SIZE, 0);
    tmpManifestID.resize(SEGMENT_ID_LENGTH, 0);
    size_t manifestListSize = 0;
    list<string> manifestList;
    while (offset != sealedDataSize) {
        memcpy(&tmpChunkFp[0], sealedDataBuffer + offset, CHUNK_HASH_SIZE);
        offset += CHUNK_HASH_SIZE;
        memcpy(&manifestListSize, sealedDataBuffer + offset, sizeof(size_t));
        offset += sizeof(size_t);
        for (size_t i = 0; i < manifestListSize; i++) {
            memcpy(&tmpManifestID[0], sealedDataBuffer + offset, SEGMENT_ID_LENGTH);
            manifestList.push_back(tmpManifestID);
            offset += SEGMENT_ID_LENGTH;
        }
        hookIndex_->insert({tmpChunkFp, manifestList});
        manifestList.clear();
    }

    free(sealedDataBuffer);
    return true;
}

/**
 * @brief start to process one batch 
 * 
 * @param segment the pointer to current segment
 * @param upOutSGX the pointer to enclave-related var 
 */
void EcallSparseIndex::ProcessOneSegment(Segment_t* segment, UpOutSGX_t* upOutSGX) {
    // the in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)upOutSGX->sgxClient;
    Recipe_t* inRecipe = &sgxClient->_inRecipe;

    // find the hook set of current segment
    set<string> currentHookSet;
    uint32_t chunkNum = segment->chunkNum;
    SegmentMeta_t* metaDataPtr = segment->metadata;
    for (size_t i = 0; i < chunkNum; i++) {
        if (this->IsHook(metaDataPtr->chunkHash)) {
            // true: add this fp to the hook list
            string hookFp((char*)metaDataPtr->chunkHash, CHUNK_HASH_SIZE);
            currentHookSet.insert(hookFp);
        }
        metaDataPtr++;
    }
    // reset the metadata pointer
    metaDataPtr = segment->metadata;
    ///

    // check sparse index to find candidate semgent list 
    unordered_map<string, uint32_t> tmpCandidateSegmentList;
    for (auto it = currentHookSet.begin(); it != currentHookSet.end(); it++) {
        auto findResult = hookIndex_->find(*it);
        if (findResult != hookIndex_->end()) {
            // this hook is existed in the hook index
            this->UpdateCandidateList(findResult->second, tmpCandidateSegmentList);
        }
    }
    /// 

    // find the top-k segment and update the deduplication index
    unordered_map<string, string> tmpDedupIndex;
    this->FindChampionSegment(tmpCandidateSegmentList, tmpDedupIndex, upOutSGX);
    ///

    // step-4: perform deduplication 
    string tmpChunkAddrStr;
    tmpChunkAddrStr.resize(sizeof(RecipeEntry_t), 0);
    bool uniqueSegmentFlag = false;
    string tmpChunkFp;
    tmpChunkFp.resize(CHUNK_HASH_SIZE, 0);
    size_t offset = 0;
    vector<BinValue_t> manifiestValueBuffer;
    BinValue_t tmpBinValue;

    for (size_t i = 0; i < chunkNum; i++) {
        // chunk hash
        tmpChunkFp.assign((char*)metaDataPtr->chunkHash, CHUNK_HASH_SIZE);

        // check the deduplication index
        auto tmpFindResult = tmpDedupIndex.find(tmpChunkFp);
        if (tmpFindResult != tmpDedupIndex.end()) {
            // it is duplicate 
            this->UpdateFileRecipe(tmpFindResult->second, inRecipe, upOutSGX);
            memcpy(tmpBinValue.chunkFp, &tmpChunkFp[0], CHUNK_HASH_SIZE);
            memcpy(&tmpBinValue.address, &tmpFindResult->second[0], sizeof(RecipeEntry_t));
            manifiestValueBuffer.push_back(tmpBinValue);
        } else {
            // it is unique 
            _uniqueDataSize += metaDataPtr->chunkSize;
            _uniqueChunkNum++;

            // process one unique chunk
            this->ProcessUniqueChunk((RecipeEntry_t*)&tmpChunkAddrStr[0],
                segment->buffer + offset, metaDataPtr->chunkSize,
                upOutSGX);
            
            // add this chunk to current deduplication index
            tmpDedupIndex.insert({tmpChunkFp, tmpChunkAddrStr});

            // update the file recipe 
            this->UpdateFileRecipe(tmpChunkAddrStr, inRecipe, upOutSGX);
            memcpy(tmpBinValue.chunkFp, &tmpChunkFp[0], CHUNK_HASH_SIZE);
            memcpy(&tmpBinValue.address, &tmpChunkAddrStr[0], sizeof(RecipeEntry_t));
            manifiestValueBuffer.push_back(tmpBinValue); 
            uniqueSegmentFlag = true;
        }

        offset += metaDataPtr->chunkSize;
        metaDataPtr++;
    }

    string manifest;
    if (uniqueSegmentFlag) {
        // update the manifest store
        manifest.resize(SEGMENT_ID_LENGTH, 0);
        Ocall_CreateUUID((uint8_t*)&manifest[0], SEGMENT_ID_LENGTH);
        if (!this->UpdateManifiestStore(manifest, manifiestValueBuffer, upOutSGX)) {
            Ocall_SGX_Exit_Error("EcallSparseIndex: cannot update the manifest store.");
        }
        ///

        // update the hook index
        this->UpdateHookIndex(manifest, currentHookSet);
        ///
    }
    
    totalSegmentNum_++;

    return ;
}