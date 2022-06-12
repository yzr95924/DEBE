/**
 * @file RAUtil.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of Remote attestion util
 * @version 0.1
 * @date 2021-05-21
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef RA_UTIL_H
#define RA_UTIL_H

#include "configure.h"
#include "chunkStructure.h"
#include "define.h"
#include "sslConnection.h"
#include "../src/Enclave/include/ocallUtil.h"

// for sdk ra header
#include "sgx_uae_epid.h"
#include "sgx_ukey_exchange.h"

#if (ENABLE_SGX_RA==1)
#include "../build/src/Enclave/storeEnclave_u.h"
#endif

//server public key
static const sgx_ec256_public_t def_service_public_key = {
    { 0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
        0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
        0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
        0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38 },
    { 0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
        0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
        0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
        0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06 }

};

extern Configure config;

class RAUtil {
    private:
        string myName_ = "RAUtil";
        // for communication
        SSLConnection* dataSecureChannel_;

        void SendMsg01RevMsg2(uint32_t extendedEPIDGroupID, sgx_ra_msg1_t& msg1,
            sgx_ra_msg2_t& msg2, SSL* newClientConnection);

        void SendMsg3(sgx_ra_msg3_t* msg3, uint32_t msg3Size, ra_msg4_t& msg4,
            SSL* newClientConnection);
        
        bool verbose_;

    public:

        RAUtil(SSLConnection* dataSecurityChannelObj);

        /**
         * @brief Destroy the RAUtil object
         * 
         */
        ~RAUtil();

        void DoAttestation(sgx_enclave_id_t eidSGX, sgx_ra_context_t& raCtx, 
            SSL* newClientConnection);
};

#endif