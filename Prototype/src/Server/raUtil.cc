/**
 * @file RAUtil.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of RA Util
 * @version 0.1
 * @date 2021-05-21
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/raUtil.h"


/**
 * @brief Construct a new RAUtil object
 * 
 */
RAUtil::RAUtil(SSLConnection* dataSecurityChannelObj) {
    dataSecureChannel_ = dataSecurityChannelObj;
    verbose_ = false;
    tool::Logging(myName_.c_str(), "init the RAUtil.\n");
}

/**
 * @brief Destroy the RAUtil object
 * 
 */
RAUtil::~RAUtil() {
}


void RAUtil::DoAttestation(sgx_enclave_id_t eidSGX, sgx_ra_context_t& raCtx,
    SSL* newClientConnection) {
#if (ENABLE_SGX_RA == 1)
    struct timeval sTime;
    struct timeval eTime;
    double totalTime = 0;

    sgx_status_t sgxStatus;
    sgx_status_t retStatus;
    sgx_status_t pseStatus;

    sgx_ra_msg1_t msg1;
    sgx_ra_msg2_t msg2;
    sgx_ra_msg3_t* msg3;

    gettimeofday(&sTime, NULL);
    // init the ra
    Ecall_Enclave_RA_Init(eidSGX, &retStatus, def_service_public_key,
        false, &raCtx, &pseStatus);

    if (retStatus != SGX_SUCCESS) {
        tool::Logging(myName_.c_str(), "RA init ret fails.\n");
        OcallUtil::PrintSGXErrorMessage(retStatus);
        exit(EXIT_FAILURE);
    }

    // generate msg0 
    uint32_t msg0_extended_epid_group_id = 0;
    sgxStatus = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);

    if (sgxStatus != SGX_SUCCESS) {
        tool::Logging(myName_.c_str(), "get msg0 extended epid fails.\n");
        OcallUtil::PrintSGXErrorMessage(sgxStatus);
        exit(EXIT_FAILURE);
    }

    // generate msg1
    sgxStatus = sgx_ra_get_msg1(raCtx, eidSGX, sgx_ra_get_ga, &msg1);
    if (sgxStatus != SGX_SUCCESS) {
        tool::Logging(myName_.c_str(), "get msg1 fails.\n");
        OcallUtil::PrintSGXErrorMessage(sgxStatus); 
        exit(EXIT_FAILURE);
    }

    if (verbose_) {
        tool::Logging(myName_.c_str(), "msg0 extended epid group id: %u\n", msg0_extended_epid_group_id);
        tool::Logging(myName_.c_str(), "msg1 gid: ");
        tool::PrintBinaryArray((uint8_t*)&msg1.gid, sizeof(msg1.gid));
    }

    // send msg0 and msg1 to the client for verification
    this->SendMsg01RevMsg2(msg0_extended_epid_group_id, msg1, msg2,
        newClientConnection);
    
    uint32_t msg3Size = 0;
    sgxStatus = sgx_ra_proc_msg2(raCtx, eidSGX, sgx_ra_proc_msg2_trusted,
        sgx_ra_get_msg3_trusted, &msg2, sizeof(sgx_ra_msg2_t) + msg2.sig_rl_size,
        &msg3, &msg3Size);

    if (sgxStatus != SGX_SUCCESS) {
        tool::Logging(myName_.c_str(), "msg2 proc error.\n");
        exit(EXIT_FAILURE);
    }

    if (verbose_) {
        tool::Logging(myName_.c_str(), "msg2 spid: ");
        tool::PrintBinaryArray((uint8_t*)&msg2.spid, sizeof(msg2.spid));
        tool::Logging(myName_.c_str(), "msg3 CMAC: ");
        tool::PrintBinaryArray((uint8_t*)&msg3->mac, sizeof(msg3->mac));
    }

    // send msg3 to the client 
    ra_msg4_t msg4;
    this->SendMsg3(msg3, msg3Size, msg4, newClientConnection);
    free(msg3);

    // read msg4
    if (msg4.status == 1) {
        tool::Logging(myName_.c_str(), "the enclave is trusted.\n");
    } else {
        tool::Logging(myName_.c_str(), "the enclave is untrusted.\n");
    }

    Ecall_Get_RA_Key_Hash(eidSGX, raCtx, SGX_RA_KEY_SK);

    // clear
    Ecall_Enclave_RA_Close(eidSGX, &retStatus, raCtx);

    gettimeofday(&eTime, NULL);

    totalTime = tool::GetTimeDiff(sTime, eTime);
    tool::Logging(myName_.c_str(), "total RA time: %lf\n", totalTime);
#endif
    return ;
}

void RAUtil::SendMsg01RevMsg2(uint32_t extendedEPIDGroupID, sgx_ra_msg1_t& msg1,
    sgx_ra_msg2_t& msg2, SSL* newClientConnection) {
    NetworkHead_t requestBody;
    requestBody.messageType = SGX_RA_MSG01;
    requestBody.dataSize = sizeof(extendedEPIDGroupID) + sizeof(msg1);

    size_t offset = 0;
    size_t totalSendSize = sizeof(NetworkHead_t) + requestBody.dataSize;
    uint8_t requestBuffer[MAX_SGX_MESSAGE_SIZE];

    // copy to the request buffer
    memcpy(requestBuffer + offset, &requestBody, sizeof(NetworkHead_t));
    offset += sizeof(NetworkHead_t);
    memcpy(requestBuffer + offset, &extendedEPIDGroupID, sizeof(extendedEPIDGroupID));
    offset += sizeof(extendedEPIDGroupID);
    memcpy(requestBuffer + offset, &msg1, sizeof(msg1));
    offset += sizeof(msg1);
    ///

    if (!dataSecureChannel_->SendData(newClientConnection, requestBuffer, totalSendSize)) {
        tool::Logging(myName_.c_str(), "send the msg0 and msg1 fails.\n");
        exit(EXIT_FAILURE);
    }

    // wait the response from the client 
    uint8_t responseBuffer[MAX_SGX_MESSAGE_SIZE];
    uint32_t recvSize = 0;
    if (!dataSecureChannel_->ReceiveData(newClientConnection, responseBuffer, recvSize)) {
        tool::Logging(myName_.c_str(), "recv the msg2 fails.\n");
        exit(EXIT_FAILURE);
    }

    NetworkHead_t responseBody; 
    responseBody.messageType = 0;
    responseBody.dataSize = 0;
    memcpy(&responseBody, responseBuffer, sizeof(NetworkHead_t));

    if (responseBody.messageType != SGX_RA_MSG2) {
        tool::Logging(myName_.c_str(), "recv the msg2 type error.\n");
        exit(EXIT_FAILURE);
    }

    // copy the data to the msg2 buffer
    memcpy(&msg2, responseBuffer + sizeof(NetworkHead_t), 
        recvSize - sizeof(NetworkHead_t));

    return ;
}


void RAUtil::SendMsg3(sgx_ra_msg3_t* msg3, uint32_t msg3Size, ra_msg4_t& msg4,
    SSL* newClientConnection) {
    NetworkHead_t requestBody;
    requestBody.messageType = SGX_RA_MSG3;
    requestBody.dataSize = msg3Size;

    size_t offset = 0;
    size_t totalSendSize = sizeof(NetworkHead_t) + requestBody.dataSize;
    uint8_t requestBuffer[MAX_SGX_MESSAGE_SIZE];

    // copy to the request buffer 
    memcpy(requestBuffer + offset, &requestBody, sizeof(NetworkHead_t));
    offset += sizeof(NetworkHead_t);
    memcpy(requestBuffer + offset, msg3, msg3Size);
    offset += msg3Size;

    if (!dataSecureChannel_->SendData(newClientConnection, requestBuffer, totalSendSize)) {
        tool::Logging(myName_.c_str(), "send the msg3 fails.\n");
        exit(EXIT_FAILURE);
    }

    // wait the response from the client
    uint8_t responseBuffer[MAX_SGX_MESSAGE_SIZE];
    uint32_t recvSize = 0;
    if (!dataSecureChannel_->ReceiveData(newClientConnection, responseBuffer, recvSize)) {
        tool::Logging(myName_.c_str(), "recv the msg4 fails.\n");
        exit(EXIT_FAILURE);
    }

    NetworkHead_t responseBody; 
    responseBody.messageType = 0;
    responseBody.dataSize = 0;
    memcpy(&responseBody, responseBuffer, sizeof(NetworkHead_t));

    if (responseBody.messageType != SGX_RA_MSG4) {
        tool::Logging(myName_.c_str(), "recv the msg2 type error.\n");
        exit(EXIT_FAILURE);
    }

    // copy the data to the msg2 buffer
    memcpy(&msg4, responseBuffer + sizeof(NetworkHead_t), 
        sizeof(ra_msg4_t));

    return ;
}