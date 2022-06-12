/**
 * @file RAECall.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of RA ECalls
 * @version 0.1
 * @date 2021-05-21
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/RAECall.h"

/* for remote attestation */
sgx_status_t Ecall_Enclave_RA_Init(sgx_ec256_public_t key, int b_pse, sgx_ra_context_t *ctx,
            sgx_status_t *pse_status) {
    sgx_status_t ra_status = SGX_SUCCESS;
#if (ENABLE_SGX_RA ==1)
    ra_status = sgx_ra_init(&key, b_pse, ctx);
#endif
	return ra_status;
}


sgx_status_t Ecall_Enclave_RA_Close(sgx_ra_context_t ctx) {
    sgx_status_t ret = SGX_SUCCESS;
#if (ENABLE_SGX_RA == 1)
    ret = sgx_ra_close(ctx);
#endif
    return ret;
}

void Ecall_Get_RA_Key_Hash(sgx_ra_context_t ctx, sgx_ra_key_type_t type) {
    string skHashStr;
    skHashStr.resize(CHUNK_ENCRYPT_KEY_SIZE);
#if (ENABLE_SGX_RA ==1)
    sgx_ra_key_128_t k;
    sgx_status_t keyStatus;
    keyStatus = sgx_ra_get_keys(ctx, type, &k);
    if (keyStatus != SGX_SUCCESS) {
        Ocall_SGX_Exit_Error("RA Get RA key fails.");
    }
    // sgx_sha256_msg((uint8_t*)&k, sizeof(k), (sgx_sha256_hash_t*)&skHashStr[0]);
    // Enclave::secretKey_.assign(skHashStr);
#endif
    return ;
}