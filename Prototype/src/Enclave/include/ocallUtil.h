/**
 * @file ocallUtil.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of the ocall util
 * @version 0.1
 * @date 2020-12-18
 * 
 * @copyright Copyright (c) 2020
 * 
 */


#ifndef OCALL_UTIL_H
#define OCALL_UTIL_H

#include <stdio.h>

#include "sgx_urts.h"

namespace OcallUtil {
    void PrintSGXErrorMessage(sgx_status_t err);
};
#endif