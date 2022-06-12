/**
 * @file enclaveSimple.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2020-12-08
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/enclaveIndex.h"

/**
 * @brief Construct a new Enclave Simple Index object
 * 
 * @param indexStore the reference to the index store
 * @param indexType the type of index
 * @param eidSGX the enclave id
 */
EnclaveIndex::EnclaveIndex(AbsDatabase* indexStore, int indexType, 
    sgx_enclave_id_t eidSGX) : AbsIndex(indexStore) {
    // import the enclave id
    eidSGX_ = eidSGX;
    Ecall_Init_Upload(eidSGX_, indexType);
    tool::Logging(myName_.c_str(), "init the EnclaveIndex.\n");
}

/**
 * @brief Destroy the Enclave Simple Index:: Enclave Simple Index object
 * 
 */
EnclaveIndex::~EnclaveIndex() {
    fprintf(stderr, "========EnclaveIndex Info========\n");
    fprintf(stderr, "recv data size: %lu\n", totalRecvDataSize_);
    fprintf(stderr, "recv batch num: %lu\n", totalBatchNum_);
    fprintf(stderr, "=================================\n");
}

/**
 * @brief process the tail segment
 * 
 * @param upOutSGX the structure to store the enclave related variable
 */
void EnclaveIndex::ProcessTailBatch(UpOutSGX_t* upOutSGX) {
    Ecall_ProcTailChunkBatch(eidSGX_, upOutSGX);
    return ;
}

/**
 * @brief process one batch
 * 
 * @param recvChunkBuf the recv chunk buffer
 * @param upOutSGX the structure to store the enclave related variable
 */
void EnclaveIndex::ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, 
    UpOutSGX_t* upOutSGX) {
    // update statistic 
    totalRecvDataSize_ += recvChunkBuf->header->dataSize;
    totalBatchNum_++;
    Ecall_ProcChunkBatch(eidSGX_, recvChunkBuf, upOutSGX);
    // update the out-enclave index, if necessary
    if (upOutSGX->outQuery->queryNum != 0) {
        Ocall_UpdateOutIndex(upOutSGX->outClient);
    }
    return ;
}