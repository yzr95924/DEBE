/**
 * @file commonEnclave.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the common structure of the enclave
 * @version 0.1
 * @date 2021-03-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef COMMON_ENCLAVE_H
#define COMMON_ENCLAVE_H

// for std using inside the enclave
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "string"
#include "stdint.h"
#include "set"
#include "unordered_map" // for deduplication index inside the enclave
#include "vector" // for heap
#include "list"
#include "mutex"

// for sgx library
#include "sgx_trts.h"
#include "../../../build/src/Enclave/storeEnclave_t.h"
#include "../../../include/chunkStructure.h"
#include "../../../include/constVar.h"
#include "ecallClient.h"

class EnclaveBase;

using namespace std;
namespace Enclave {
    void Logging(const char* logger, const char* fmt, ...);
    void WriteBufferToFile(uint8_t* buffer, size_t bufferSize, const char* fileName);
    void ReadFileToBuffer(uint8_t* buffer, size_t bufferSize, const char* fileName);
    extern unordered_map<int, string> clientSessionKeyIndex_;
    extern uint8_t* enclaveKey_;
    extern uint8_t* indexQueryKey_; 
    extern bool firstBootstrap_; // use to control the RA
    // config
    extern uint64_t sendChunkBatchSize_;
    extern uint64_t sendRecipeBatchSize_;
    extern uint64_t topKParam_;
    // mutex
    extern mutex sessionKeyLck_;
    extern mutex sketchLck_;
    extern mutex topKIndexLck_;
    // the obj to the enclave index
    extern EnclaveBase* enclaveBaseObj_;
};

#endif