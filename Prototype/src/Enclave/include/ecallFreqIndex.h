/**
 * @file ecallFreqTwoIndex.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of freq two-path index
 * @version 0.1
 * @date 2021-01-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ECALL_FREQ_TWO_INDEX_H
#define ECALL_FREQ_TWO_INDEX_H

#include "enclaveBase.h"
#include "ecallCMSketch.h"
#include "ecallEntryHeap.h"

#define SEALED_FREQ_INDEX "freq-index"
#define SEALED_SKETCH "cm-sketch"

class EcallFreqIndex : public EnclaveBase {
    private:
        string myName_ = "EcallFreqIndex";

        // the top-k threshold
        size_t topThreshold_;

        // the pointer to the cm-sketch inside the enclave
        EcallCMSketch* cmSketch_;

        // the width of the sketch
        size_t sketchWidth_ = 256 * 1024;
        size_t sketchDepth_ = 4;

        // the deduplication index
        EcallEntryHeap* insideDedupIndex_;

        uint64_t insideDedupChunkNum_ = 0;
        uint64_t insideDedupDataSize_ = 0;

        /**
         * @brief update the inside-enclave with only freq
         * 
         * @param ChunkFp the chunk fp
         * @param currentFreq the current frequency
         */
        void UpdateInsideIndexFreq(const string& chunkFp, uint32_t currentFreq);

        /**
         * @brief check whether add this chunk to the heap
         * 
         * @param chunkFreq the chunk freq
         */
        bool CheckIfAddToHeap(uint32_t chunkFreq);

        /**
         * @brief Add the information of this chunk to the heap
         * 
         * @param chunkFreq the chunk freq
         * @param chunkAddr the chunk address
         * @param chunkFp the chunk fp
         */
        void AddChunkToHeap(uint32_t chunkFreq, RecipeEntry_t* chunkAddr, const string& chunkFp);

        /**
         * @brief persist the deduplication index into the disk
         * 
         * @return true success
         * @return false fail
         */
        bool PersistDedupIndex();

        /**
         * @brief read the hook index from sealed data
         * 
         * @return true success
         * @return false fail
         */
        bool LoadDedupIndex();
    public:

        /**
         * @brief Construct a new Ecall Frequency Index object
         * 
         */
        EcallFreqIndex();

        /**
         * @brief Destroy the Ecall Frequency Index object
         * 
         */
        ~EcallFreqIndex();

        /**
         * @brief process one batch
         * 
         * @param buffer the recv chunk buffer
         * @param upOutSGX the pointer to the enclave-related var 
         */
        void ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf,
            UpOutSGX_t* upOutSGX);

        /**
         * @brief process the tailed batch when received the end of the recipe flag
         * 
         * @param upOutSGX the pointer to enclave-related var
         */
        void ProcessTailBatch(UpOutSGX_t* upOutSGX);
};

#endif