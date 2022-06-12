/**
 * @file RAVerifier.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces RA verifier 
 * @version 0.1
 * @date 2021-05-22
 * 
 * @copyright Copyright (c) 2021
 * 
 */


#ifndef RA_VERIFIER_H
#define RA_VERIFIER_H

#include "configure.h"
#include "define.h"
#include "chunkStructure.h"
#include "sslConnection.h"
#include "cryptoPrimitive.h"

// for RA verification
#include "IAS/base64.h"
#include "IAS/byteorder.h"
#include "IAS/json.h"
#include "IAS/crypto.h"
#include "IAS/iasrequest.h"
#include "IAS/hexutil.h"

#include "sgx_quote.h"

#define CA_BUNDLE "/etc/ssl/certs/ca-certificates.crt"
#define IAS_SIGNING_CA_FILE "../key/IASRootCA.pem"

typedef struct {
    uint8_t enclaveTrusted;
    uint8_t g_a[64];
    uint8_t g_b[64];
    uint8_t kdk[16];
    uint8_t smk[16];
    uint8_t sk[16];
    uint8_t mk[16];
    uint8_t vk[16];
    sgx_ra_msg1_t msg1;
} EnclaveSession_t;

typedef struct {
    uint32_t msg0_extended_epid_group_id;
    sgx_ra_msg1_t msg1;
} SGX_Msg01_t;

static const unsigned char def_service_private_key[32] = {
    0x90, 0xe7, 0x6c, 0xbb, 0x2d, 0x52, 0xa1, 0xce,
    0x3b, 0x66, 0xde, 0x11, 0x43, 0x9c, 0x87, 0xec,
    0x1f, 0x86, 0x6a, 0x3b, 0x65, 0xb6, 0xae, 0xea,
    0xad, 0x57, 0x34, 0x53, 0xd1, 0x03, 0x8c, 0x01
};

extern Configure config;

class RAVerifier {
    private:
        SSLConnection* dataSecureChannel_;

        void ProcessMsg01(EnclaveSession_t& enclaveSession, SGX_Msg01_t& recvMsg01,
            sgx_ra_msg2_t& msg2);

        void ProcessMsg3(EnclaveSession_t& enclaveSession, sgx_ra_msg3_t* msg3,
            ra_msg4_t& msg4, uint32_t quote_sz);
        
        void DeriveKDK(EVP_PKEY* Gb, uint8_t* kdk, sgx_ec256_public_t g_a);

        void GetSigrl(uint8_t* gid, char* sig_rl, uint32_t* sig_rl_size);

        void GetAttestationReport(const char* b64quote, sgx_ps_sec_prop_desc_t secprop,
            ra_msg4_t* msg4);

        bool verbose_;

        // for RA configuration
        sgx_spid_t spid_;
        uint16_t quoteType_;
        IAS_Connection* iasConnection_;

        X509_STORE* caStore_;
        X509* signingCA_;
        EVP_PKEY* servicePrivateKey_;
        uint16_t iasVersion_;

        CryptoPrimitive* cryptoObj_;
        EVP_MD_CTX* mdCtx_;
        EVP_CIPHER_CTX* cipherCtx_;
        EnclaveSession_t currentSession_;

    public:
        RAVerifier(SSLConnection* dataSecureChannel);

        ~RAVerifier();

        void RAVerification(SSL* connectionForServer, EnclaveSession_t& newSession);
};

#endif