/**
 * @file topkTwoIndex.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in the topk-two
 * @version 0.1
 * @date 2021-01-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/freqIndex.h"

/**
 * @brief Construct a new Top K Two Index object
 * 
 * @param inputFile the trace file
 * @param type FSL or MS 
 * @param k 
 */
FreqIndex::FreqIndex(string inputFile, int type, uint32_t k) : 
    AbsIndex(inputFile, type) {
    k_ = k;
    cmSketch_ = new CountMinSketch(width_, depth_);
    newMinHeap_ = new EcallEntryHeap();
    newMinHeap_->SetHeapSize(k_);
    logicalChunkDB_ = dbFactory_.CreateDatabase(LEVEL_DB, "plaintext-db");
    cipherChunkDB_ = dbFactory_.CreateDatabase(LEVEL_DB, "ciphertext-db"); 
    tool::Logging(myName_.c_str(), "init the FreqIndex.\n");
}


/**
 * @brief process the trace file
 * 
 * @return true success
 * @return false fail
 */
FreqIndex::~FreqIndex() {
    delete logicalChunkDB_;
    delete cipherChunkDB_;
    delete cmSketch_;
    delete newMinHeap_;
    fprintf(stderr, "========FreqIndex Info========\n");
    fprintf(stderr, "Original KLD (CE): %lf\n", originalKLD_);
    fprintf(stderr, "Cipher KLD (DEBE): %lf\n", newKLD_);
    fprintf(stderr, "==============================\n");
}


/**
 * @brief process the trace file
 * 
 * @return true success
 * @return false fail
 */
bool FreqIndex::ProcessTrace() {
    tool::Logging(myName_.c_str(), "start to process %s\n", inputFile_.c_str());
    string inputLine;
    char readLineBuffer[256];
    vector<ChunkInfo> fpBatchBuffer;

    while (getline(fpIn_, inputLine)) {
        // read trace file line by line
        memset(readLineBuffer, 0, 256);
        memcpy(readLineBuffer, inputLine.c_str(), inputLine.length());

        uint8_t chunkFp[chunkHashLen + 1];
        memset(chunkFp, 0, chunkHashLen + 1);

        uint64_t chunkSize = this->ParseChunkInformation(readLineBuffer, chunkFp);

        if (chunkSize > MAX_CHUNK_SIZE) {
            chunkSize = MAX_CHUNK_SIZE;
        }

        // update logical data
        logicalDataSize_ += chunkSize;
        logicalChunkNum_++;

        // count the frequency of logical chunk 
        this->CountChunkFreq(chunkFp, LOGICAL_CHUNK, chunkSize);

        // add this chunk to the chunk batch buffer
        ChunkInfo tmpChunkInfo;
        tmpChunkInfo.fpStr.assign((char*)chunkFp, chunkHashLen);
        tmpChunkInfo.chunkSize = chunkSize;
        fpBatchBuffer.push_back(tmpChunkInfo);

        if (fpBatchBuffer.size() % 1024 == 0) {
            this->ProcessBatchFp(fpBatchBuffer);
            fpBatchBuffer.clear();
        } else {
            continue;
        }
    }
    if (fpBatchBuffer.size() != 0) {
        this->ProcessBatchFp(fpBatchBuffer);
        fpBatchBuffer.clear();
    }    

    this->ComputeKLD();
    this->ClearUp();
    return true;
}

/**
 * @brief update the inside-enclave with only freq
 * 
 * @param ChunkFp the chunk fp
 * @param currentFreq the current frequency
 */
void FreqIndex::UpdateInsideIndexFreq(const string& chunkFp, uint32_t currentFreq) {
    // uint32_t tmpHeapEntry;
    // newMinHeap_->GetPriority(chunkFp, tmpHeapEntry);
    // tmpHeapEntry = currentFreq;
    newMinHeap_->Update(chunkFp, currentFreq);
    return ;
}

/**
 * @brief process a batch of the fp
 * 
 * @param fpBatchBuffer 
 */
void FreqIndex::ProcessBatchFp(vector<ChunkInfo>& fpBatchBuffer) {
    unordered_map<string, ChunkStatus> tmpIndex;
    vector<uint8_t> dedupStatusRecord;
    uint32_t currentMinFreq;

    // get the current min-freq
    if (newMinHeap_->Size() != 0) {
        currentMinFreq = newMinHeap_->TopEntry();
    } else {
        currentMinFreq = 0;
    }

    for (int i = 0; i < fpBatchBuffer.size(); i++) {
        uint32_t currentFreq;
        // update the sketch
        string chunkFp = fpBatchBuffer[i].fpStr;
        cmSketch_->Update((uint8_t*)&chunkFp[0], chunkHashLen, 1);
        // estimate the current frequency
        currentFreq = cmSketch_->Estimate((uint8_t*)&chunkFp[0], chunkHashLen);

        // for batching merge them together in a batch index
        auto localFindResult = tmpIndex.find(chunkFp);
        if (localFindResult != tmpIndex.end()) {
            // it exists in this batch
            uint8_t status = dedupStatusRecord[localFindResult->second.offset];
            switch (status) {
                case UNIQUE: {
                    // this chunk is unique for the heap, but duplicate for local index
                    dedupStatusRecord.push_back(TMP_UNIQUE);
                    break;
                }
                case DUPLICATE: {
                    // this is duplicate for heap, also duplicate for the local index
                    dedupStatusRecord.push_back(TMP_DUPLICATE);
                    break;
                }
                default: {
                    tool::Logging(myName_.c_str(), "wrong dedup flag.\n");
                    exit(EXIT_FAILURE);
                }
            }

            // update the frequency in the batch index
            localFindResult->second.freq = currentFreq;
        } else {
            // it does not exist in the batch index, compare the freq
            ChunkStatus tmpStatus;
            if (currentFreq < currentMinFreq) {
                // its frequency is smaller than the minimum value in the heap, must not exist in the heap
                // need to quey outside
                tmpStatus.freq = currentFreq;
                tmpStatus.offset = dedupStatusRecord.size();
                dedupStatusRecord.push_back(UNIQUE);
            } else {
                // its frequency is higher than the minimum value in the heap, check the heap
                bool heapFindResult = newMinHeap_->Contains(chunkFp);
                if (heapFindResult) {
                    // it exists in the current min heap
                    tmpStatus.freq = currentFreq;
                    tmpStatus.offset = dedupStatusRecord.size();
                    dedupStatusRecord.push_back(DUPLICATE);
                } else {
                    // it does not exist in the current min heap, need to query the outside index
                    tmpStatus.freq = currentFreq;
                    tmpStatus.offset = dedupStatusRecord.size();
                    dedupStatusRecord.push_back(UNIQUE);
                }
            }
            tmpIndex[chunkFp] = tmpStatus;
        }
    }

    // query the outside index
    for (int i = 0; i < dedupStatusRecord.size(); i++) {
        if (dedupStatusRecord[i] == UNIQUE) {
            // need to query the outside index
            string fp = fpBatchBuffer[i].fpStr;
            uint32_t chunkSize = fpBatchBuffer[i].chunkSize;

            this->CountChunkFreq((uint8_t*)&fp[0], CIPHER_CHUNK, chunkSize);
            // auto findOutsideResult = outsideDedupSet_.find(fp);
            // if (findOutsideResult != outsideDedupSet_.end()) {
            //     // it is a duplicate for the outside index
            //     outsideDedupChunkNum_++;
            //     outsideDedupDataSize_ += chunkSize;
            // } else {
            //     // it also does not exist in the outside index
            //     uniqueChunkNum_++;
            //     uniqueDataSize_ += chunkSize;

            //     // update the outside index
            //     outsideDedupSet_.insert(fp);
            // }
        } else {
            continue;
        }
    }

    // update the min-heap
    for (int i = 0; i < fpBatchBuffer.size(); i++) {
        string fp = fpBatchBuffer[i].fpStr;
        uint32_t chunkSize = fpBatchBuffer[i].chunkSize;
        uint32_t currentFreq = tmpIndex[fp].freq;
        switch (dedupStatusRecord[i]) {
            case DUPLICATE: {
                if (newMinHeap_->Contains(fp)) {
                    // it exists in the min-heap
                    this->UpdateInsideIndexFreq(fp, currentFreq); 
                } else {
                    // it does not exist in the min-heap
                    if (newMinHeap_->Size() == k_) {
                        // remove the root node
                        uint32_t heapMinFreq = newMinHeap_->TopEntry();
                        if (currentFreq >= heapMinFreq) {
                            // add this chunk to the heap
                            newMinHeap_->Pop();
                            
                            // push the new item
                            HeapItem_t newItem;
                            newItem.chunkFreq = currentFreq;
                            newMinHeap_->Add(fp, newItem);
                            replaceHappensTime_++;
                        } 
                    } else {
                        // directly add the node
                        HeapItem_t newItem;
                        newItem.chunkFreq = currentFreq;
                        newMinHeap_->Add(fp, newItem);
                    }
                }
                insideDedupChunkNum_++;
                insideDedupDataSize_ += chunkSize;
                break;
            }
            case TMP_DUPLICATE: {
                insideDedupChunkNum_++;
                insideDedupDataSize_ += chunkSize;
                break;
            }
            case UNIQUE: {
                // it does not exist in the min-heap
                if (newMinHeap_->Contains(fp)) {
                    // it exists in the min-heap
                    this->UpdateInsideIndexFreq(fp, currentFreq); 
                } else {
                    // it does not exist in the min-heap
                    if (newMinHeap_->Size() == k_) {
                        // remove the root node
                        uint32_t heapMinFreq = newMinHeap_->TopEntry();
                        if (currentFreq >= heapMinFreq) {
                            // add this chunk to the heap
                            newMinHeap_->Pop();
                            
                            // push the new item
                            HeapItem_t newItem;
                            newItem.chunkFreq = currentFreq;
                            newMinHeap_->Add(fp, newItem);
                            replaceHappensTime_++;
                        } 
                    } else {
                        // directly add the node
                        HeapItem_t newItem;
                        newItem.chunkFreq = currentFreq;
                        newMinHeap_->Add(fp, newItem);
                    }
                }
                break;
            }
            case TMP_UNIQUE: {
                insideDedupChunkNum_++;
                insideDedupDataSize_ += chunkSize;
                break;
            }
        }
    }
    return ;
}