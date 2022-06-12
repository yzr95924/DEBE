/**
 * @file restoreEcall.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the ecall of restore
 * @version 0.1
 * @date 2021-03-21
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef RESTORE_ECALL_H
#define RESTORE_ECALL_H

#include "commonEnclave.h"

#include "ecallEnc.h"
#include "ecallRecvDecoder.h"

class EcallRecvDecoder;

namespace RestoreEnclave {
    // the pointer to the enclave-based recv decoder
    extern EcallRecvDecoder* ecallRecvDecoderObj_;
};

using namespace RestoreEnclave;

/**
 * @brief init the init the ecall restore 
 * 
 */
void Ecall_Init_Restore();

/**
 * @brief destore the restore enclave memory
 * 
 */
void Ecall_Destroy_Restore();

/**
 * @brief decode the recipe inside the enclave
 * 
 * @param recipeBuffer the recipe buffer
 * @param recipeNum the input recipe number
 * @param resOutSGX the pointer to the out-enclave var
 * 
 * @return size_t the size of the restored buffer
 */
void Ecall_ProcRecipeBatch(uint8_t* recipeBuffer, size_t recipeNum,
    ResOutSGX_t* resOutSGX);

/**
 * @brief decode the tail recipe inside the enclave
 * 
 * @param resOutSGX the pointer to the out-enclave var
 */
void Ecall_ProcRecipeTailBatch(ResOutSGX_t* resOutSGX);

#endif