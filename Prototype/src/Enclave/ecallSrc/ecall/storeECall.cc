/**
 * @file encECall.cpp
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the ECALLs of StoreEnclave 
 * @version 0.1
 * @date 2020-10-02
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/storeECall.h"

using namespace Enclave;

/**
 * @brief init the ecall
 * 
 * @param indexType the type of the index
 */
void Ecall_Init_Upload(int indexType) {
    // choose different types of indexes here
    switch (indexType) {
        case OUT_ENCLAVE: {
            enclaveBaseObj_ = new EcallOutEnclaveIndex();
            break;
        }
        case EXTREME_BIN: {
            enclaveBaseObj_ = new EcallExtremeBinIndex();
            break;
        }
        case SPARSE_INDEX: {
            enclaveBaseObj_ = new EcallSparseIndex();
            break;
        }
        case FREQ_INDEX: {
            enclaveBaseObj_ = new EcallFreqIndex();
            break;
        }
        case IN_ENCLAVE: {
            enclaveBaseObj_ = new EcallInEnclaveIndex();
            break;
        }
        default:
            Ocall_SGX_Exit_Error("wrong enclave index type.");
    }

    size_t readFileSize = 0;
    Ocall_InitReadSealedFile(&readFileSize, ENCLAVE_INDEX_INFO_NAME);
    if (readFileSize != 0) {
        // the file exists, read previous stastics info
        uint8_t* infoBuf = (uint8_t*) malloc(sizeof(uint64_t) * 5);
        size_t offset = 0;
        Ocall_ReadSealedData(ENCLAVE_INDEX_INFO_NAME, infoBuf, 
            sizeof(uint64_t) * 5);
        memcpy(&enclaveBaseObj_->_logicalDataSize, infoBuf + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(&enclaveBaseObj_->_logicalChunkNum, infoBuf + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(&enclaveBaseObj_->_uniqueDataSize, infoBuf + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(&enclaveBaseObj_->_uniqueChunkNum, infoBuf + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(&enclaveBaseObj_->_compressedDataSize, infoBuf + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        free(infoBuf);
    }
    Ocall_CloseReadSealedFile(ENCLAVE_INDEX_INFO_NAME);
    return ;
}

/**
 * @brief destore the enclave memory 
 * 
 * @return bool 
 */
void Ecall_Destroy_Upload() {
    // persist the enclave info here
    bool ret;
    Ocall_InitWriteSealedFile(&ret, ENCLAVE_INDEX_INFO_NAME);
    if (ret != true) {
        Ocall_SGX_Exit_Error("cannot open the enclave index info file.");
    } 
    Ocall_WriteSealedData(ENCLAVE_INDEX_INFO_NAME, 
        (uint8_t*)&enclaveBaseObj_->_logicalDataSize, sizeof(uint64_t));
    Ocall_WriteSealedData(ENCLAVE_INDEX_INFO_NAME, 
        (uint8_t*)&enclaveBaseObj_->_logicalChunkNum, sizeof(uint64_t));
    Ocall_WriteSealedData(ENCLAVE_INDEX_INFO_NAME, 
        (uint8_t*)&enclaveBaseObj_->_uniqueDataSize, sizeof(uint64_t));
    Ocall_WriteSealedData(ENCLAVE_INDEX_INFO_NAME, 
        (uint8_t*)&enclaveBaseObj_->_uniqueChunkNum, sizeof(uint64_t));
    Ocall_WriteSealedData(ENCLAVE_INDEX_INFO_NAME, 
        (uint8_t*)&enclaveBaseObj_->_compressedDataSize, sizeof(uint64_t));
    Ocall_CloseWriteSealedFile(ENCLAVE_KEY_FILE_NAME);
    if (enclaveBaseObj_) {
        delete enclaveBaseObj_;
    }
    return ;
}

/**
 * @brief process one batch of chunk
 * 
 * @param recvChunkBuf the recv chunk buffer
 * @param upOutSGX the pointer to enclave-needed structure
 */
void Ecall_ProcChunkBatch(SendMsgBuffer_t* recvChunkBuf, UpOutSGX_t* upOutSGX) {
    enclaveBaseObj_->ProcessOneBatch(recvChunkBuf, upOutSGX);
    return ;
}

/**
 * @brief process the tail batch 
 * 
 * @param upOutSGX the pointer to enclave-needed structure 
 */
void Ecall_ProcTailChunkBatch(UpOutSGX_t* upOutSGX) {
    enclaveBaseObj_->ProcessTailBatch(upOutSGX);
    return ;
}

/**
 * @brief init the inside client var
 * 
 * @param clientID the client ID
 * @param type the index type 
 * @param optType the operation type (upload/download)
 * @param encMasterKey the encrypted master key 
 * @param sgxClient the pointer to the sgx client
 */
void Ecall_Init_Client(uint32_t clientID, int type, int optType, 
    uint8_t* encMasterKey, void** sgxClient) {
    // allocate a client resource inside the enclave
    EnclaveClient* newClient = new EnclaveClient(clientID, type, optType);

{
    // check the session key for this users 
#if (MULTI_CLIENT == 1)
    Enclave::sessionKeyLck_.lock();
#endif
    auto findResult = Enclave::clientSessionKeyIndex_.find(clientID);
    if (findResult == Enclave::clientSessionKeyIndex_.end()) {
        Ocall_SGX_Exit_Error("get the session key fails.");
    } else {
        // can find the session key, directly set the session key of this client
        memcpy(newClient->_sessionKey, &findResult->second[0],
            CHUNK_HASH_SIZE);
    }
#if (MULTI_CLIENT == 1)
    Enclave::sessionKeyLck_.unlock();
#endif
}

    // set the client master key
    newClient->SetMasterKey(encMasterKey, CHUNK_HASH_SIZE);
    // return the sgx client ptr
    memcpy(sgxClient, &newClient, sizeof(EnclaveClient*));
    return ;
}

/**
 * @brief destroy the inside client var
 * 
 * @param sgxClient the sgx-client ptr
 */
void Ecall_Destroy_Client(void* sgxClient) {
    // destroy the client resource inside the enclave
    EnclaveClient* sgxClientPtr = (EnclaveClient*)sgxClient;
    delete sgxClientPtr;
    return ;
}

/**
 * @brief init the enclave 
 * 
 * @param enclaveConfig the enclave config
 */
void Ecall_Enclave_Init(EnclaveConfig_t* enclaveConfig) {
    using namespace Enclave;
    // set the enclave key and index query key (this can be configured by RA)
    enclaveKey_ = (uint8_t*) malloc(sizeof(uint8_t) * CHUNK_HASH_SIZE);
    indexQueryKey_ = (uint8_t*) malloc(sizeof(uint8_t) * CHUNK_HASH_SIZE);
    firstBootstrap_ = true;

    // config
    sendChunkBatchSize_ = enclaveConfig->sendChunkBatchSize;
    sendRecipeBatchSize_ = enclaveConfig->sendRecipeBatchSize;
    topKParam_ = enclaveConfig->topKParam;

    // check the file 
    size_t readFileSize = 0;
    Ocall_InitReadSealedFile(&readFileSize, ENCLAVE_KEY_FILE_NAME);
    if (readFileSize == 0) {
        // file not exists
        firstBootstrap_ = true;

        // randomly generate the data key, query key, and the global secret
        sgx_read_rand(enclaveKey_, CHUNK_HASH_SIZE);
        sgx_read_rand(indexQueryKey_, CHUNK_HASH_SIZE);
    } else {
        // file exists
        firstBootstrap_ = false;
        uint8_t* readBuffer = (uint8_t*) malloc(CHUNK_HASH_SIZE * 2);
        Ocall_ReadSealedData(ENCLAVE_KEY_FILE_NAME, readBuffer, CHUNK_HASH_SIZE * 2);
        memcpy(enclaveKey_, readBuffer, CHUNK_HASH_SIZE);
        memcpy(indexQueryKey_, readBuffer + CHUNK_HASH_SIZE, 
            CHUNK_HASH_SIZE);
        free(readBuffer);
    }
    Ocall_CloseReadSealedFile(ENCLAVE_KEY_FILE_NAME);
    return ;
}

/**
 * @brief destroy the enclave
 * 
 */
void Ecall_Enclave_Destroy() {
    using namespace Enclave;
    // check the status 
    if (firstBootstrap_ == true) {
        // perisit the key
        bool ret;
        Ocall_InitWriteSealedFile(&ret, ENCLAVE_KEY_FILE_NAME);
        if (ret != true) {
            Ocall_SGX_Exit_Error("cannot open the enclave key file.");
        } 
        Ocall_WriteSealedData(ENCLAVE_KEY_FILE_NAME, enclaveKey_, 
            CHUNK_HASH_SIZE);
        Ocall_WriteSealedData(ENCLAVE_KEY_FILE_NAME, indexQueryKey_,
            CHUNK_HASH_SIZE);
        Ocall_CloseWriteSealedFile(ENCLAVE_KEY_FILE_NAME);
    }
    // free the enclave key, index query key and the global secret
    free(enclaveKey_); 
    free(indexQueryKey_);
    return ;
}

/**
 * @brief get the enclave info 
 * 
 * @param info the enclave info
 */
void Ecall_GetEnclaveInfo(EnclaveInfo_t* info) {
    info->logicalDataSize = enclaveBaseObj_->_logicalDataSize;
    info->logicalChunkNum = enclaveBaseObj_->_logicalChunkNum;
    info->uniqueDataSize = enclaveBaseObj_->_uniqueDataSize;
    info->uniqueChunkNum = enclaveBaseObj_->_uniqueChunkNum;
    info->compressedSize = enclaveBaseObj_->_compressedDataSize;
#if (SGX_BREAKDOWN == 1)
    double rawOcallTime = enclaveBaseObj_->_testOCallTime / 
        static_cast<double>(enclaveBaseObj_->_testOCallCount);
    double MS_TO_USEC = 1000.0;
    info->dataTranTime = (enclaveBaseObj_->_dataTransTime - 
        (enclaveBaseObj_->_dataTransCount * rawOcallTime)) / MS_TO_USEC;
    info->fpTime = (enclaveBaseObj_->_fingerprintTime - 
        (enclaveBaseObj_->_fingerprintCount * rawOcallTime)) / MS_TO_USEC;
    info->freqTime = (enclaveBaseObj_->_freqTime -
        (enclaveBaseObj_->_freqCount * rawOcallTime)) / MS_TO_USEC;
    info->firstDedupTime = (enclaveBaseObj_->_firstDedupTime - 
        (enclaveBaseObj_->_firstDedupCount * rawOcallTime)) / MS_TO_USEC;
    info->secondDedupTime = (enclaveBaseObj_->_secondDedupTime - 
        (enclaveBaseObj_->_secondDedupCount * rawOcallTime)) / MS_TO_USEC;
    info->compTime = (enclaveBaseObj_->_compressTime - 
        (enclaveBaseObj_->_compressCount * rawOcallTime)) / MS_TO_USEC;
    info->encTime = (enclaveBaseObj_->_encryptTime - 
        (enclaveBaseObj_->_encryptCount * rawOcallTime)) / MS_TO_USEC;
    
    // reset count
    enclaveBaseObj_->_testOCallCount = 0;
    enclaveBaseObj_->_dataTransCount = 0;
    enclaveBaseObj_->_fingerprintCount = 0;
    enclaveBaseObj_->_freqCount = 0;
    enclaveBaseObj_->_firstDedupCount = 0;
    enclaveBaseObj_->_secondDedupCount = 0;
    enclaveBaseObj_->_compressCount = 0;
    enclaveBaseObj_->_encryptCount = 0;

    // reset time
    enclaveBaseObj_->_testOCallTime = 0;
    enclaveBaseObj_->_dataTransTime = 0;
    enclaveBaseObj_->_fingerprintTime = 0;
    enclaveBaseObj_->_freqTime = 0;
    enclaveBaseObj_->_firstDedupTime = 0;
    enclaveBaseObj_->_secondDedupTime = 0;
    enclaveBaseObj_->_compressTime = 0;
    enclaveBaseObj_->_encryptTime = 0;
#endif
    return ;
}