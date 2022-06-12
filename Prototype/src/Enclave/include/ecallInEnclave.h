/**
 * @file ecallBaseline.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of baseline full-enclave index
 * @version 0.1
 * @date 2021-05-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ECALL_BASE_LINE
#define ECALL_BASE_LINE

#include "enclaveBase.h"

#define SEALED_BASELINE_INDEX_PATH "baseline-seal-index"

class EcallInEnclaveIndex : public EnclaveBase {
    private:
        string myName_ = "EcallInEnclaveIndex";
        // for the inside index
        unordered_map<string, string> insideIndexObj_;

        /**
         * @brief persist the deduplication index to the disk
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
         * @brief Construct a new Ecall Baseline Index object
         * 
         */
        EcallInEnclaveIndex();

        /**
         * @brief Destroy the Ecall Baseline Index object
         * 
         */
        ~EcallInEnclaveIndex();

        /**
         * @brief process one batch
         * 
         * @param recvChunkBuf the recv chunk buffer
         * @param upOutSGX the pointer to enclave-related var
         */
        void ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, UpOutSGX_t* upOutSGX);

        /**
         * @brief process the tailed batch when received the end of the recipe flag
         * 
         * @param upOutSGX the pointer to enclave-related var
         */
        void ProcessTailBatch(UpOutSGX_t* upOutSGX);
};

#endif