/**
 * @file encECall.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the ECALL interface in storeEnclave 
 * @version 0.1
 * @date 2020-10-02
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef STORE_ECALL_H 
#define STORE_ECALL_H

#include "commonEnclave.h"

// for the enclave only
#include "ecallEnc.h"

// for the ecall index
#include "enclaveBase.h"
#include "ecallOutEnclave.h"
#include "ecallInEnclave.h"
#include "ecallExtreme.h"
#include "ecallSparse.h"
#include "ecallFreqIndex.h"

// for ecall store
#include "ecallStorage.h"

#define ENCLAVE_KEY_FILE_NAME "enclave-key"
#define ENCLAVE_INDEX_INFO_NAME "enclave-index-info"

namespace Enclave {
    // variable stated here must be "extern"
    // the pointer to the enclave base
    extern EnclaveBase* enclaveBaseObj_;
};

/**
 * @brief init the ecall
 * 
 * @param indexType the type of the index
 */
void Ecall_Init_Upload(int indexType);

/**
 * @brief destore the enclave memory 
 * 
 */
void Ecall_Destroy_Upload();

/**
 * @brief process one batch of chunk
 * 
 * @param recvChunkBuf the recv chunk buffer
 * @param upOutSGX the pointer to enclave-needed structure
 */
void Ecall_ProcChunkBatch(SendMsgBuffer_t* recvChunkBuf,
    UpOutSGX_t* upOutSGX);

/**
 * @brief process the tail batch 
 * 
 * @param upOutSGX the pointer to enclave-needed structure 
 */
void Ecall_ProcTailChunkBatch(UpOutSGX_t* upOutSGX);

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
    uint8_t* encMasterKey, void** sgxClient);

/**
 * @brief destroy the inside client var
 * 
 * @param sgxClient the sgx-client ptr
 */
void Ecall_Destroy_Client(void* sgxClient);

/**
 * @brief init the enclave 
 * 
 * @param enclaveConfig the enclave config
 */
void Ecall_Enclave_Init(EnclaveConfig_t* enclaveConfig);

/**
 * @brief destroy the enclave
 * 
 */
void Ecall_Enclave_Destroy();

/**
 * @brief get the enclave info 
 * 
 * @param info the enclave info
 */
void Ecall_GetEnclaveInfo(EnclaveInfo_t* info);

#endif // ENC_ECALL_H