/**
 * @file restoreEcall.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the inteface of restore ocall
 * @version 0.1
 * @date 2021-03-22
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/restoreECall.h"

namespace RestoreEnclave {
    EcallRecvDecoder* ecallRecvDecoderObj_ = NULL;
};

using namespace RestoreEnclave;

/**
 * @brief init the init the ecall restore 
 * 
 * @param enclaveConfig the pointer to the enclave config
 */
void Ecall_Init_Restore() {
    ecallRecvDecoderObj_ = new EcallRecvDecoder();
    return ;
}

/**
 * @brief destore the restore enclave memory
 * 
 */
void Ecall_Destroy_Restore() {
    if (ecallRecvDecoderObj_) {
        delete ecallRecvDecoderObj_;
    }
    return ;
}

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
    ResOutSGX_t* resOutSGX) {
    ecallRecvDecoderObj_->ProcRecipeBatch(recipeBuffer, recipeNum, 
        resOutSGX);
    return ;
}

/**
 * @brief decode the tail recipe inside the enclave
 * 
 * @param resOutSGX the pointer to the out-enclave var
 */
void Ecall_ProcRecipeTailBatch(ResOutSGX_t* resOutSGX) {
    ecallRecvDecoderObj_->ProcRecipeTailBatch(resOutSGX);
    return ;
}