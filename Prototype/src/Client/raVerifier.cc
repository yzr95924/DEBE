/**
 * @file raVerifier.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of raVerifier
 * @version 0.1
 * @date 2021-05-22
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/raVerifier.h"



RAVerifier::RAVerifier(SSLConnection* dataSecureChannel) {
    dataSecureChannel_ = dataSecureChannel;
    verbose_ = false;
    if (!cert_load_file(&signingCA_, IAS_SIGNING_CA_FILE)) {
        fprintf(stderr, "RAverifier: cannot load IAS signing cert.\n");
        exit(EXIT_FAILURE);
    }

    caStore_ = cert_init_ca(signingCA_);
    if (!caStore_) {
        fprintf(stderr, "RAVerifer: cannot init cert file store.\n");
        exit(EXIT_FAILURE);
    }

    // config spid
    string spidStr = config.GetSPID();
    if (spidStr.length() != 32) {
        fprintf(stderr, "RAVerifier: the spid has wrong format.\n");
        exit(EXIT_FAILURE);
    }
    from_hexstring((uint8_t*)&spid_, (void*)&spidStr[0], 16);

    iasConnection_ = new IAS_Connection(config.GetIASServerType(),
        0,
        (char*)(&config.GetIASPrimaryKey()[0]),
        (char*)(&config.GetIASSecKey()[0]));
    iasConnection_->agent("wget");
#if (TEST_IN_CSE == 1)
    iasConnection_->proxy_mode(IAS_PROXY_FORCE);
#else
    iasConnection_->proxy_mode(IAS_PROXY_NONE);
#endif
    iasConnection_->cert_store(caStore_);
    iasConnection_->ca_bundle(CA_BUNDLE);

    iasVersion_ = config.GetIASVersion();
    servicePrivateKey_ = key_private_from_bytes(def_service_private_key);
    quoteType_ = config.GetQuoteType();

    cryptoObj_ = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);
    mdCtx_ = EVP_MD_CTX_new();
    cipherCtx_ = EVP_CIPHER_CTX_new();

    fprintf(stderr, "RAVerifier: Init the RAVerifier.\n");
}

RAVerifier::~RAVerifier() {
    delete cryptoObj_;
    EVP_MD_CTX_free(mdCtx_);
    EVP_CIPHER_CTX_free(cipherCtx_);
    X509_STORE_free(caStore_);
    X509_free(signingCA_);
    EVP_PKEY_free(servicePrivateKey_);
    delete iasConnection_;
    fprintf(stderr, "RAVerifier: Destroy the RAVerifier.\n");
}

void RAVerifier::RAVerification(SSL* connectionForServer, EnclaveSession_t& newSession) {
    SGX_Msg01_t recvMsg01; 

    // for msg2
    sgx_ra_msg2_t msg2;
    ra_msg4_t msg4;

    uint8_t responseBuffer[MAX_SGX_MESSAGE_SIZE];
    NetworkHead_t requestHeader;
    NetworkHead_t responseHeader;
    uint32_t recvSize = 0;
    size_t offset = 0;
    uint32_t totalSendSize = 0;

    // recv msg01
    if (!dataSecureChannel_->ReceiveData(connectionForServer, responseBuffer,
        recvSize)) {
        fprintf(stderr, "RAVerifier: recv msg01 fails.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(&requestHeader, responseBuffer + offset, sizeof(requestHeader));
    offset += sizeof(requestHeader);
    if (requestHeader.messageType != SGX_RA_MSG01) {
        fprintf(stderr, "RAVerifier: recv wrong msg01 type.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(&recvMsg01, responseBuffer + offset, sizeof(recvMsg01));

    // process msg01 and generate msg2
    this->ProcessMsg01(newSession, recvMsg01, msg2);
    responseHeader.messageType = SGX_RA_MSG2;
    responseHeader.dataSize = sizeof(msg2) + msg2.sig_rl_size;

    offset = 0;
    memcpy(responseBuffer + offset, &responseHeader, sizeof(responseHeader)); 
    offset += sizeof(responseHeader);
    memcpy(responseBuffer + offset, &msg2, responseHeader.dataSize);
    offset += responseHeader.dataSize;
    totalSendSize = sizeof(NetworkHead_t) + responseHeader.dataSize;
    if (!dataSecureChannel_->SendData(connectionForServer, responseBuffer, 
        totalSendSize)) {
        fprintf(stderr, "RAVerifier: send msg2 fails.\n");
        exit(EXIT_FAILURE);
    }

    // recv msg3 
    if (!dataSecureChannel_->ReceiveData(connectionForServer, responseBuffer,
        recvSize)) {
        fprintf(stderr, "RAVerifier: recv msg3 fails.\n");
        exit(EXIT_FAILURE);
    }
    offset = 0;
    memcpy(&requestHeader, responseBuffer + offset, sizeof(requestHeader));
    offset += sizeof(requestHeader);
    if (requestHeader.messageType != SGX_RA_MSG3) {
        fprintf(stderr, "RAVerifier: recv msg3 wrong type.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t quote_size = recvSize - sizeof(NetworkHead_t) - 
        sizeof(sgx_ra_msg3_t);
    if (verbose_) {
        fprintf(stderr, "RAVerifier: recv msg3 quote size: %u\n", quote_size);
    }

    sgx_ra_msg3_t* msg3Buffer = (sgx_ra_msg3_t*) (responseBuffer + offset);

    this->ProcessMsg3(newSession, msg3Buffer, msg4, quote_size);

    requestHeader.messageType = SGX_RA_MSG4;
    requestHeader.dataSize = sizeof(msg4);
    totalSendSize = sizeof(NetworkHead_t) + requestHeader.dataSize;

    offset = 0;
    memcpy(responseBuffer + offset, &requestHeader, sizeof(requestHeader));
    offset += sizeof(requestHeader);
    memcpy(responseBuffer + offset, &msg4, sizeof(msg4));
    offset += sizeof(msg4);

    if (!dataSecureChannel_->SendData(connectionForServer, responseBuffer, 
        totalSendSize)) {
        fprintf(stderr, "RAVerifer: send msg4 error.\n");
        exit(EXIT_FAILURE);
    }

    memcpy(&currentSession_, &newSession, sizeof(EnclaveSession_t));
    fprintf(stderr, "RAVerifier: RA is successful.\n");

    return ;
}

void RAVerifier::ProcessMsg01(EnclaveSession_t& enclaveSession, SGX_Msg01_t& recvMsg01,
    sgx_ra_msg2_t& msg2) {
    EVP_PKEY* Gb;
    uint8_t digest[32];
    uint8_t r[32];
    uint8_t s[32];
    uint8_t gb_ga[128];

    if (verbose_) {
        fprintf(stderr, "RAVerifier: the recv msg0 group ID: %u\n", recvMsg01.msg0_extended_epid_group_id);
        fprintf(stderr, "RAVerifier: msg1.gid: ");
        tool::PrintBinaryArray((uint8_t*)&recvMsg01.msg1.gid,
            sizeof(recvMsg01.msg1.gid));
    }
    if (recvMsg01.msg0_extended_epid_group_id != 0) {
        fprintf(stderr, "RAVerifier: msg0 extended epid group ID is not zero.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(&enclaveSession.msg1, &recvMsg01.msg1, sizeof(recvMsg01.msg1));

    Gb = key_generate();
    if (!Gb) {
        fprintf(stderr, "RAVerifier: cannot crate session key.\n");
        exit(EXIT_FAILURE);
    }

    this->DeriveKDK(Gb, enclaveSession.kdk, recvMsg01.msg1.g_a);

    cmac128(enclaveSession.kdk, (uint8_t*)("\x01SMK\x00\x80\x00"), 7,
        enclaveSession.smk);

    // generate msg2
    memset(&msg2, 0, sizeof(sgx_ra_msg2_t));

    key_to_sgx_ec256(&msg2.g_b, Gb);
    memcpy(&msg2.spid, &spid_, sizeof(sgx_spid_t));
    msg2.quote_type = quoteType_;
    msg2.kdf_id = 1;

    // get the sigrl
    this->GetSigrl(recvMsg01.msg1.gid, (char*)&msg2.sig_rl, &msg2.sig_rl_size);

    memcpy(gb_ga, &msg2.g_b, 64);
    memcpy(enclaveSession.g_b, &msg2.g_b, 64);

    memcpy(&gb_ga[64], &enclaveSession.msg1.g_a, 64);
    memcpy(enclaveSession.g_a, &enclaveSession.msg1.g_a, 64);

    ecdsa_sign(gb_ga, 128, servicePrivateKey_, r, s, digest);
    reverse_bytes(&msg2.sign_gb_ga.x, r, 32);
    reverse_bytes(&msg2.sign_gb_ga.y, s, 32);

    cmac128(enclaveSession.smk, (uint8_t*)&msg2, 148,
        (uint8_t*)&msg2.mac);

    EVP_PKEY_free(Gb);    
    return ;
}

void RAVerifier::DeriveKDK(EVP_PKEY* Gb, uint8_t* kdk, sgx_ec256_public_t g_a) {
    uint8_t* Gab_x;
    uint8_t cmacKey[16];
    size_t len;
    EVP_PKEY* Ga;

    // compute the share secret
    Ga = key_from_sgx_ec256(&g_a);
    if (!Ga) {
        fprintf(stderr, "RAVerifier: cannot get the ga from msg1.\n");
        exit(EXIT_FAILURE);
    }

    Gab_x = key_shared_secret(Gb, Ga, &len);
    if (!Gab_x) {
        fprintf(stderr, "RAVerifier: cannot get the shared secret.\n");
        exit(EXIT_FAILURE);
    }
    reverse_bytes(Gab_x, Gab_x, len);

    memset(cmacKey, 0, sizeof(cmacKey));
    cmac128(cmacKey, Gab_x, len, kdk);

    // free memory
    EVP_PKEY_free(Ga);
    OPENSSL_free(Gab_x);
    return ;
}

void RAVerifier::GetSigrl(uint8_t* gid, char* sig_rl, uint32_t* sig_rl_size) {
    IAS_Request* req;
    req = new IAS_Request(iasConnection_, iasVersion_);
    if (!req) {
        fprintf(stderr, "RAVerifier: cannot make isa request.\n");
        exit(EXIT_FAILURE);
    }

    string sigrlStr;
    if (req->sigrl(*(uint32_t*)gid, sigrlStr) != IAS_OK) {
        fprintf(stderr, "RAVerifier: ias get sigrl error.\n");
        exit(EXIT_FAILURE);
    }

    memcpy(sig_rl, &sigrlStr[0], sigrlStr.size());
    if (!sig_rl) {
        fprintf(stderr, "RAVerifier: get sig_rl fails.\n");
        exit(EXIT_FAILURE);
    }
    *sig_rl_size = (uint32_t)sigrlStr.size();
    delete req;
    return ;
}

void RAVerifier::ProcessMsg3(EnclaveSession_t& enclaveSession, sgx_ra_msg3_t* msg3,
    ra_msg4_t& msg4, uint32_t quote_sz) {
    if (verbose_) {
        fprintf(stderr, "RAVerifier: msg3 CMAC: ");
        tool::PrintBinaryArray((uint8_t*)&msg3->mac, sizeof(msg3->mac));
    }

    if (CRYPTO_memcmp(&msg3->g_a, &enclaveSession.msg1.g_a,
        sizeof(sgx_ec256_public_t))) {
        fprintf(stderr, "RAVerifier: msg1.ga != msg3.ga\n");
        exit(EXIT_FAILURE);
    }

    // validate the MAC 
    sgx_mac_t vrfMsgMAC;
    cmac128(enclaveSession.smk, (uint8_t*)&msg3->g_a,
        sizeof(sgx_ra_msg3_t) - sizeof(sgx_mac_t) + quote_sz,
        (uint8_t*)vrfMsgMAC);
    
    if (verbose_) {
        fprintf(stderr, "RAVerifier: msg3.mac = %s\n", 
            hexstring(msg3->mac, sizeof(sgx_mac_t)));
        fprintf(stderr, "RAVerifier: calculated = %s\n",
            hexstring(vrfMsgMAC, sizeof(sgx_mac_t)));
    }

    if (CRYPTO_memcmp(msg3->mac, vrfMsgMAC, sizeof(sgx_mac_t))) {
        fprintf(stderr, "RAVerifier: failed to verify msg3 mac\n");
        exit(EXIT_FAILURE);
    }

    char* b64quote;
    b64quote = base64_encode((char*)&msg3->quote, quote_sz);
    sgx_quote_t* q;
    q = (sgx_quote_t*) msg3->quote;
    if (memcmp(enclaveSession.msg1.gid, &q->epid_group_id, 
        sizeof(sgx_epid_group_id_t))) {
        fprintf(stderr, "RAVerifier: attestation fails (differ gid).\n");
        exit(EXIT_FAILURE);
    }

    this->GetAttestationReport(b64quote, msg3->ps_sec_prop, &msg4);

    uint8_t vfy_rdata[64];
    uint8_t msg_rdata[144]; /* for Ga || Gb || VK */

    sgx_report_body_t* r = (sgx_report_body_t*)&q->report_body;

    memset(vfy_rdata, 0, 64);

    // derive vk
    cmac128(enclaveSession.kdk, (unsigned char*)("\x01VK\x00\x80\x00"),
        6, enclaveSession.vk);
    
    // build our plaintext
    memcpy(msg_rdata, enclaveSession.g_a, 64);
    memcpy(&msg_rdata[64], enclaveSession.g_b, 64);
    memcpy(&msg_rdata[128], enclaveSession.vk, 16);

    // sha-256 hash
    sha256_digest(msg_rdata, 144, vfy_rdata);

    if (CRYPTO_memcmp((void*)vfy_rdata, (void*)&r->report_data,
        64)) {
        fprintf(stderr, "RAVerifier: report verification failed.\n");
        exit(EXIT_FAILURE);
    }

    if (msg4.status) {
        cmac128(enclaveSession.kdk, (unsigned char*)("\x01MK\x00\x80\x00"),
            6, enclaveSession.mk);
        cmac128(enclaveSession.kdk, (unsigned char*)("\x01SK\x00\x80\x00"),
            6, enclaveSession.sk);
        enclaveSession.enclaveTrusted = true;
    }

    free(b64quote);
    return ;
}


void RAVerifier::GetAttestationReport(const char* b64quote, sgx_ps_sec_prop_desc_t secprop,
    ra_msg4_t* msg4) {
    IAS_Request* req = NULL;
    map<string, string> payload;
    vector<string> messages;
    ias_error_t status;
    string content;

    req = new IAS_Request(iasConnection_, iasVersion_);
    if (!req) {
        fprintf(stderr, "RAVerifier: create IAS request object fails.\n");
        exit(EXIT_FAILURE);
    }

    payload.insert(make_pair("isvEnclaveQuote", b64quote));

    status = req->report(payload, content, messages);
    if (status == IAS_OK) {
        using namespace json;
        JSON reportObj = JSON::Load(content);

        /*
         * If the report returned a version number (API v3 and above), make
         * sure it matches the API version we used to fetch the report.
         *
         * For API v3 and up, this field MUST be in the report.
         */

        if (reportObj.hasKey("version")) {
            uint32_t retVersion = (uint32_t)reportObj["version"].ToInt();
            if(iasVersion_ != retVersion) {
                fprintf(stderr, "RAVerifier: report version cannot match.\n");
                exit(EXIT_FAILURE);
            }
        }

        memset(msg4, 0, sizeof(ra_msg4_t));

        if (!(reportObj["isvEnclaveQuoteStatus"].ToString().compare("OK"))) {
            msg4->status = true;
        } else if (!(reportObj["isvEnclaveQuoteStatus"].ToString().compare("CONFIGURATION_NEEDED"))) {
            msg4->status = true;
        } else if (!(reportObj["isvEnclaveQuoteStatus"].ToString().compare("GROUP_OUT_OF_DATE"))) {
            msg4->status = true;
        } else {
            msg4->status = false;
        }
    }

    messages.clear();
    delete req;
    return ;
}