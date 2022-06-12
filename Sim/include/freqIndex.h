/**
 * @file freqIndex.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2021-01-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef TOP_K_TWO_INDEX_H
#define TOP_K_TWO_INDEX_H

#include "define.h"
#include "absIndex.h"
#include "cmSketch.h"
#include "absDatabase.h"
#include "ecallEntryHeap.h"

using namespace std;

typedef struct {
    string fpStr;
    uint32_t chunkSize;
} ChunkInfo;

typedef struct {
    uint32_t freq;
    uint32_t offset;
} ChunkStatus;

class FreqIndex: public AbsIndex {
    private:
        string myName_ = "FreqIndex";
        CountMinSketch* cmSketch_;

        // dedup set 
        unordered_set<string> outsideDedupSet_;

        uint32_t width_ = 256 * 1024;
        uint32_t depth_ = 4;
        
        // the deduplication index
        EcallEntryHeap* newMinHeap_;

        // statistics
        uint64_t insideDedupChunkNum_ = 0;
        uint64_t insideDedupDataSize_ = 0;
        // uint64_t outsideDedupChunkNum_ = 0;
        // uint64_t outsideDedupDataSize_ = 0;

        uint64_t replaceHappensTime_ = 0;

        /**
         * @brief update the inside-enclave with only freq
         * 
         * @param ChunkFp the chunk fp
         * @param currentFreq the current frequency
         */
        void UpdateInsideIndexFreq(const string& chunkFp, uint32_t currentFreq);

        /**
         * @brief process a batch of the fp
         * 
         * @param fpBatchBuffer 
         */
        void ProcessBatchFp(vector<ChunkInfo>& fpBatchBuffer);

    public:
        /**
         * @brief Construct a new Top K Two Index object
         * 
         * @param inputFile the trace file
         * @param type FSL or MS 
         * @param k 
         */
        FreqIndex(string inputFile, int type, uint32_t k);

        /**
         * @brief process the trace file
         * 
         * @return true success
         * @return false fail
         */
        bool ProcessTrace();

        /**
         * @brief Destroy the Top K Two Index object
         * 
         */
        ~FreqIndex();
};

#endif