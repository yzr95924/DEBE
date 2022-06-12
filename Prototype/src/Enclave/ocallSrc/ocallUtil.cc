/**
 * @file ocallUtil.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the basic interface of ocall 
 * @version 0.1
 * @date 2020-12-09
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../include/ocallUtil.h"

/**
 * @brief print the sgx error message
 * 
 * @param err the error code
 */
void OcallUtil::PrintSGXErrorMessage(sgx_status_t err) {
    switch(err) {
        case SGX_ERROR_INVALID_PARAMETER:             
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_PARAMETER\n", (int) err);
            break;
        case SGX_ERROR_INVALID_CPUSVN: 
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_CPUSVN\n", (int) err);
            break;
        case SGX_ERROR_INVALID_ISVSVN: 
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_ISVSVN\n", (int) err);
            break;
        case SGX_ERROR_MAC_MISMATCH: 
            fprintf(stderr, "[%d] SGX_ERROR_MAC_MISMATCH\n", (int) err);
            break;
        case SGX_ERROR_OUT_OF_MEMORY: 
            fprintf(stderr, "[%d] SGX_ERROR_OUT_OF_MEMORY\n", (int) err);
            break;
        case SGX_ERROR_UNEXPECTED: 
            fprintf(stderr, "[%d] SGX_ERROR_UNEXPECTED\n", (int) err);
            break;
        case SGX_ERROR_ENCLAVE_LOST:
            fprintf(stderr, "[%d] SGX_ERROR_ENCLAVE_LOST\n", (int) err);
            break;
        case SGX_ERROR_INVALID_STATE:
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_STATE\n", (int) err);
            break;
        case SGX_ERROR_INVALID_FUNCTION:
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_FUNCTION\n", (int) err);
            break;
        case SGX_ERROR_OUT_OF_TCS:
            fprintf(stderr, "[%d] SGX_ERROR_OUT_OF_TCS\n", (int) err);
            break;
        case SGX_ERROR_ENCLAVE_CRASHED:
            fprintf(stderr, "[%d] SGX_ERROR_ENCLAVE_CRASHED\n", (int) err);
            break;
        case SGX_ERROR_ECALL_NOT_ALLOWED:
            fprintf(stderr, "[%d] SGX_ERROR_ECALL_NOT_ALLOWED\n", (int) err);
            break;
        case SGX_ERROR_OCALL_NOT_ALLOWED:
            fprintf(stderr, "[%d] SGX_ERROR_OCALL_NOT_ALLOWED\n", (int) err);
            break;
        case SGX_ERROR_STACK_OVERRUN:
            fprintf(stderr, "[%d] SGX_ERROR_STACK_OVERRUN\n", (int) err);
            break;
        case SGX_ERROR_UNDEFINED_SYMBOL:
            fprintf(stderr, "[%d] SGX_ERROR_UNDEFINED_SYMBOL\n", (int) err);
            break;
        case SGX_ERROR_INVALID_ENCLAVE:
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_ENCLAVE\n", (int) err);
            break;
        case SGX_ERROR_INVALID_ENCLAVE_ID:
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_ENCLAVE_ID\n", (int) err);
            break;
        case SGX_ERROR_INVALID_SIGNATURE:
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_SIGNATURE\n", (int) err);
            break;
        case SGX_ERROR_NDEBUG_ENCLAVE:
            fprintf(stderr, "[%d] SGX_ERROR_NDEBUG_ENCLAVE\n", (int) err);
            break;
        case SGX_ERROR_OUT_OF_EPC:
            fprintf(stderr, "[%d] SGX_ERROR_OUT_OF_EPC\n", (int) err);
            break;
        case SGX_ERROR_NO_DEVICE:
            fprintf(stderr, "[%d] SGX_ERROR_NO_DEVICE\n", (int) err);
            break;
        case SGX_ERROR_MEMORY_MAP_CONFLICT:
            fprintf(stderr, "[%d] SGX_ERROR_MEMORY_MAP_CONFLICT\n", (int) err);
            break;
        case SGX_ERROR_INVALID_METADATA:
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_METADATA\n", (int) err);
            break;
        case SGX_ERROR_DEVICE_BUSY:
            fprintf(stderr, "[%d] SGX_ERROR_DEVICE_BUSY\n", (int) err);
            break;
        case SGX_ERROR_INVALID_VERSION:
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_VERSION\n", (int) err);
            break;
        case SGX_ERROR_MODE_INCOMPATIBLE:
            fprintf(stderr, "[%d] SGX_ERROR_MODE_INCOMPATIBLE\n", (int) err);
            break;
        case SGX_ERROR_ENCLAVE_FILE_ACCESS:
            fprintf(stderr, "[%d] SGX_ERROR_ENCLAVE_FILE_ACCESS\n", (int) err);
            break;
        case SGX_ERROR_INVALID_MISC:
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_MISC\n", (int) err);
            break;
        case SGX_ERROR_INVALID_ATTRIBUTE:
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_ATTRIBUTE\n", (int) err);
            break;
        case SGX_ERROR_INVALID_KEYNAME:
            fprintf(stderr, "[%d] SGX_ERROR_INVALID_KEYNAME\n", (int) err);
            break;
        case SGX_ERROR_SERVICE_UNAVAILABLE:
            fprintf(stderr, "[%d] SGX_ERROR_SERVICE_UNAVAILABLE\n", (int) err);
            break;
        case SGX_ERROR_SERVICE_TIMEOUT:
            fprintf(stderr, "[%d] SGX_ERROR_SERVICE_TIMEOUT\n", (int) err);
            break;
        case SGX_ERROR_AE_INVALID_EPIDBLOB:
            fprintf(stderr, "[%d] SGX_ERROR_AE_INVALID_EPIDBLOB\n", (int) err);
            break;
        case SGX_ERROR_SERVICE_INVALID_PRIVILEGE:
            fprintf(stderr, "[%d] SGX_ERROR_SERVICE_INVALID_PRIVILEGE\n", (int) err);
            break;
        case SGX_ERROR_EPID_MEMBER_REVOKED:
            fprintf(stderr, "[%d] SGX_ERROR_EPID_MEMBER_REVOKED\n", (int) err);
            break;
        case SGX_ERROR_UPDATE_NEEDED:
            fprintf(stderr, "[%d] SGX_ERROR_UPDATE_NEEDED\n", (int) err);
            break;
        case SGX_ERROR_NETWORK_FAILURE:
            fprintf(stderr, "[%d] SGX_ERROR_NETWORK_FAILURE\n", (int) err);
            break;
        case SGX_ERROR_AE_SESSION_INVALID:
            fprintf(stderr, "[%d] SGX_ERROR_AE_SESSION_INVALID\n", (int) err);
            break;
        case SGX_ERROR_BUSY:
            fprintf(stderr, "[%d] SGX_ERROR_BUSY\n", (int) err);
            break;
        case SGX_ERROR_MC_NOT_FOUND:
            fprintf(stderr, "[%d] SGX_ERROR_MC_NOT_FOUND\n", (int) err);
            break;
        case SGX_ERROR_MC_NO_ACCESS_RIGHT:
            fprintf(stderr, "[%d] SGX_ERROR_MC_NO_ACCESS_RIGHT\n", (int) err);
            break;
        case SGX_ERROR_MC_USED_UP:
            fprintf(stderr, "[%d] SGX_ERROR_MC_USED_UP\n", (int) err);
            break;
        case SGX_ERROR_MC_OVER_QUOTA:
            fprintf(stderr, "[%d] SGX_ERROR_MC_OVER_QUOTA\n", (int) err);
            break;
        case SGX_ERROR_KDF_MISMATCH:
            fprintf(stderr, "[%d] SGX_ERROR_KDF_MISMATCH\n", (int) err);
            break;
        case SGX_ERROR_UNRECOGNIZED_PLATFORM:
            fprintf(stderr, "[%d] SGX_ERROR_UNRECOGNIZED_PLATFORM\n", (int) err);
            break;
        case SGX_ERROR_NO_PRIVILEGE:
            fprintf(stderr, "[%d] SGX_ERROR_NO_PRIVILEGE\n", (int) err);
            break;
        case SGX_ERROR_FILE_BAD_STATUS:
            fprintf(stderr, "[%d] SGX_ERROR_FILE_BAD_STATUS\n", (int) err);
            break;
        case SGX_ERROR_FILE_NO_KEY_ID:
            fprintf(stderr, "[%d] SGX_ERROR_FILE_NO_KEY_ID\n", (int) err);
            break;
        case SGX_ERROR_FILE_NAME_MISMATCH:
            fprintf(stderr, "[%d] SGX_ERROR_FILE_NAME_MISMATCH\n", (int) err);
            break;
        case SGX_ERROR_FILE_NOT_SGX_FILE:
            fprintf(stderr, "[%d] SGX_ERROR_FILE_NOT_SGX_FILE\n", (int) err);
            break;
        case SGX_ERROR_FILE_CANT_OPEN_RECOVERY_FILE:
            fprintf(stderr, "[%d] SGX_ERROR_FILE_CANT_OPEN_RECOVERY_FILE\n", (int) err);
            break;
        case SGX_ERROR_FILE_CANT_WRITE_RECOVERY_FILE:
            fprintf(stderr, "[%d] SGX_ERROR_FILE_CANT_WRITE_RECOVERY_FILE\n", (int) err);
            break;
        case SGX_ERROR_FILE_RECOVERY_NEEDED:
            fprintf(stderr, "[%d] SGX_ERROR_FILE_RECOVERY_NEEDED\n", (int) err);
            break;
        case SGX_ERROR_FILE_FLUSH_FAILED:
            fprintf(stderr, "[%d] SGX_ERROR_FILE_FLUSH_FAILED\n", (int) err);
            break;
        case SGX_ERROR_FILE_CLOSE_FAILED:
            fprintf(stderr, "[%d] SGX_ERROR_FILE_CLOSE_FAILED\n", (int) err);
            break;
        case SGX_SUCCESS:
            break;
        default:
            fprintf(stderr, "[%d] sgx error\n", err);
            break;
    }
}