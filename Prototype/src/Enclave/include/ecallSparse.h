/**
 * @file ecallSparseIndex.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of sparse index
 * @version 0.1
 * @date 2021-01-01
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ECALL_SPARSE_INDEX
#define ECALL_SPARSE_INDEX

#include "enclaveBase.h"

#define SEALED_SPARSE_INDEX "sparse-seal-index"

class EcallSparseIndex : public EnclaveBase {
    private:
        string myName_ = "EcallSparseIndex";
        // the sparse index: <hook: list of manifest id>
        unordered_map<string, list<string>>* hookIndex_;

        // for statistics
        uint64_t totalSegmentNum_ = 0;

        /**
         * @brief judge whether the chunk is the hook
         * 
         * @param chunkFp chunk fingerprint
         * @return true is the hook of this segment
         * @return false is not the hook of this segment
         */
        bool IsHook(const uint8_t* chunkFp);

        /**
         * @brief update the manifest candidate table
         * 
         * @param inputSet the manifest set of the given hook
         * @param candidateSegmentList the candidate segment list 
         */
        void UpdateCandidateList(list<string>& inputSet, 
            unordered_map<string, uint32_t>& candidateSegmentList);

        /**
         * @brief find the top-k segment, and load the manifest to the deduplication index
         * 
         * @param candidateSegmentList the candidate segment list 
         * @param dedupIndex the deduplication index
         * @param upOutSGX the pointer to the enclave var
         */
        void FindChampionSegment(unordered_map<string, uint32_t>& candidateSegmentList,
            unordered_map<string, string>& dedupIndex, UpOutSGX_t* upOutSGX);

        /**
         * @brief update the deduplication index according to the segment list
         * 
         * @param segmentID the segment ID 
         * @param dedupIndex the reference to the deduplication index
         * @param upOutSGX the pointer to the enclave var
         */
        void LoadManifiest(string segmentID, unordered_map<string, string>& dedupIndex,
            UpOutSGX_t* upOutSGX);

        /**
         * @brief update the manifest store according to the manifest value buffer
         * 
         * @param manifestID the ID of the current manifest
         * @param currentBinValueBuffer the reference to the bin value buffer 
         * @param upOutSGX the pointer to the enclave var
         * @return true success
         * @return false fails
         */
        bool UpdateManifiestStore(string& manifestID, const vector<BinValue_t>& currentBinValueBuffer,
            UpOutSGX_t* upOutSGX);

        /**
         * @brief update the hook index for the given manifestID
         * 
         * @param manifest the ID of current manifest
         * @param currentHookSet the current hook set
         */
        void UpdateHookIndex(string& manifest, set<string>& currentHookSet);

        /**
         * @brief persist the hook index into disk
         * 
         * @return true success
         * @return false fail
         */
        bool PersistHookIndex();

        /**
         * @brief read the hook index from sealed data
         * 
         * @return true success
         * @return false fail
         */
        bool LoadHookIndex();

        /**
         * @brief start to process one batch 
         * 
         * @param segment the pointer to current segment
         * @param upOutSGX the pointer to enclave-related var 
         */
        void ProcessOneSegment(Segment_t* segment, UpOutSGX_t* upOutSGX);

    public:

        /**
         * @brief Construct a new Ecall Sparse Index object
         * 
         * @param enclaveConfig the pointer to the enclave config
         */
        EcallSparseIndex();

        /**
         * @brief Destroy the Ecall Sparse Index object
         * 
         */
        ~EcallSparseIndex();

        /**
         * @brief process one batch
         * 
         * @param recvChunkBuf the recv chunk buffer
         * @param upOutSGX the pointer to enclave-related var 
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