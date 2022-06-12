/**
 * @file commonEnclave.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the 
 * @version 0.1
 * @date 2020-12-21
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/commonEnclave.h"

namespace Enclave {
    unordered_map<int, string> clientSessionKeyIndex_;
    uint8_t* enclaveKey_;
    uint8_t* indexQueryKey_;
    bool firstBootstrap_; // 
    // config
    uint64_t sendChunkBatchSize_;
    uint64_t sendRecipeBatchSize_;
    uint64_t topKParam_;
    // lock
    mutex sessionKeyLck_;
    mutex sketchLck_;
    mutex topKIndexLck_;
    // the obj to the enclave index
    EnclaveBase* enclaveBaseObj_;
};

void Enclave::Logging(const char* logger, const char* fmt, ...) {
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    string output;
    output = "<" + string(logger) + ">: " + string(buf);
    Ocall_Printf(output.c_str());
}

/**
 * @brief write the buffer to the file
 * 
 * @param buffer the pointer to the buffer
 * @param bufferSize the buffer size
 * @param fileName the file name
 */
void Enclave::WriteBufferToFile(uint8_t* buffer, size_t bufferSize, const char* fileName) {
    size_t remainSize = bufferSize;
    size_t offset = 0;
    while (remainSize != 0) {
        if (remainSize > SGX_PERSISTENCE_BUFFER_SIZE) {
            Ocall_WriteSealedData(fileName, buffer + offset, SGX_PERSISTENCE_BUFFER_SIZE);
            offset += SGX_PERSISTENCE_BUFFER_SIZE;
            remainSize -= SGX_PERSISTENCE_BUFFER_SIZE;
        } else {
            Ocall_WriteSealedData(fileName, buffer + offset, remainSize);
            offset += remainSize;
            remainSize -= remainSize;
            break;
        }
    }
    return ;
}

/**
 * @brief read the file to the buffer
 * 
 * @param buffer the pointer to the buffer
 * @param bufferSize the buffer size
 * @param fileName the file name
 */
void Enclave::ReadFileToBuffer(uint8_t* buffer, size_t bufferSize, const char* fileName) {
    size_t remainSize = bufferSize;
    size_t offset = 0;
    while (remainSize != 0) {
        if (remainSize > SGX_PERSISTENCE_BUFFER_SIZE) {
            Ocall_ReadSealedData(fileName, buffer + offset, SGX_PERSISTENCE_BUFFER_SIZE); 
            offset += SGX_PERSISTENCE_BUFFER_SIZE;
            remainSize -= SGX_PERSISTENCE_BUFFER_SIZE;
        } else {
            Ocall_ReadSealedData(fileName, buffer + offset, remainSize);
            offset += remainSize;
            remainSize -= remainSize;
            break;
        }
    }
    return ;
}