/**
 * @file storeOCall.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief virtual OCall for comparsion
 * @version 0.1
 * @date 2022-04-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef STORE_OCALL_H
#define STORE_OCALL_H

#include "clientVar.h"
#include "dataWriter.h"

namespace OutEnclave {
    // TODO:
    extern string myName_; 
}

/**
 * @brief dump the inside container to the outside buffer
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_WriteContainer(void* outClient);

#endif