/**
 * @file RAECall.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of RA Ecall
 * @version 0.1
 * @date 2021-05-21
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef RA_ECALL_H
#define RA_ECALL_H

// for the enclave only
#include "commonEnclave.h"
#include "ecallEnc.h"

// for the key exchange
#include "sgx_tkey_exchange.h"
#include "sgx_utils.h"
#include "sgx_tcrypto.h"

#define PSE_RETRIES 5 /* Arbitrary. Not too long, not too short. */

/* for remote attestation */
sgx_status_t Ecall_Enclave_RA_Init(sgx_ec256_public_t key, int b_pse, sgx_ra_context_t *ctx,
            sgx_status_t *pse_status);


sgx_status_t Ecall_Enclave_RA_Close(sgx_ra_context_t ctx);

void Ecall_Get_RA_Key_Hash(sgx_ra_context_t ctx, sgx_ra_key_type_t type);

#endif