/**
 * @file ecallExtreme.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of the extreme binning index inside the enclave
 * @version 0.1
 * @date 2020-12-30
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef ECALL_EXTREME_INDEX_H
#define ECALL_EXTREME_INDEX_H

#include "enclaveBase.h"

#define SEALED_PRIMAR_INDEX "primary-seal-index"

class EcallExtremeBinIndex : public EnclaveBase {
    private:
        string myName_ = "EcallExtremeBinIndex";
        // for primary index
        unordered_map<string, string>* primaryIndex_;

        // for statistics
        uint64_t totalSegmentNum_ = 0;
        uint64_t uniqueSegmentNum_ = 0;
        uint64_t differentSegmentNum_ = 0;
        uint64_t duplicateSegmentNum_ = 0;

        /**
         * @brief fetch the bin value in to the dedup index
         * 
         * @param binAddress the bin address
         * @param tmpDedupIndex the reference to the tmp dedup index
         * @param upOutSGX the pointer to the enclave var
         * @return true successes 
         * @return false fails
         */
        bool FetchBin(string& binAddress, unordered_map<string, string>& tmpDedupIndex,
            UpOutSGX_t* upOutSGX);

        /**
         * @brief update the bin value to the store
         * 
         * @param binAddress the bin address
         * @param tmpDedupIndex the reference to the tmp dedup index
         * @param upOutSGX the pointer to the enclcave var
         * @return true successes
         * @return false fails
         */
        bool UpdateBinStore(string& binAddress, unordered_map<string, string>& tmpDedupIndex,
            UpOutSGX_t* upOutSGX);

        /**
         * @brief read the primary index from the sealed data
         * 
         * @return true success
         * @return false fail
         */
        bool LoadPrimaryIndex();

        /** * @brief seal the primary index to the disk * 
                 * @return true success
         * @return false fail
         */
        bool PersistPrimaryIndex();

        /**
         * @brief start to process one batch
         * 
         * @param segment the pointer to current segment
         * @param upOutSGX the pointer to enclave var 
         */
        void ProcessOneSegment(Segment_t* segment, 
            UpOutSGX_t* upOutSGX);
    public:
        /**
         * @brief Construct a new Ecall Extreme Index object
         * 
         */
        EcallExtremeBinIndex();

        /**
         * @brief Destroy the Ecall Extreme Index object
         * 
         */
        ~EcallExtremeBinIndex();

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