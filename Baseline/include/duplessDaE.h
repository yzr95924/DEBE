/**
 * @file duplessDaE.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of DupLESS DaE approach
 * @version 0.1
 * @date 2021-12-14
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef DUPLESS_DAE_H
#define DUPLESS_DAE_H

#include "absDaE.h"

// for blind RSA
#include <openssl/rsa.h>
#include <openssl/pem.h>

class DupLESSDAE : public AbsDAE {
    private:
        string myName_ = "DupLESS-DAE";
        const BIGNUM* clientKeyN_;
        const BIGNUM* clientKeyE_;

        SendMsgBuffer_t sendBlindFPBuf_;

        // for RSA
        BN_CTX* bnCTX_;
        RSA* clientRSA_;
        EVP_PKEY* publicKey_;

        /**
         * @brief decorate the FP
         * 
         * @param r 
         * @param fp the input fp
         * @param outputBuffer the output buffer
         */
        void DecorateFP(BIGNUM* r, uint8_t* fp, uint8_t* outputBuffer);

        /**
         * @brief eliminate the FP
         * 
         * @param inv
         * @param key 
         * @param output 
         */
        void Elimination(BIGNUM* inv, uint8_t* key, uint8_t* output);

    public:
        /**
         * @brief Construct a new DupLESSDAE object
         * 
         */
        DupLESSDAE(uint8_t* fileNameHash);

        /**
         * @brief Destroy the DupLESSDAE object
         * 
         */
        ~DupLESSDAE();

        /**
         * @brief process one segment of chunk
         * 
         * @param sendPlainChunkBuf the buffer to the plaintext chunk
         * @param sendCipherChunkBuf the buffer to the ciphertext chunk
         * @param sendRecipeBuf the buffer for the recipe <only store the key>
         */
        void ProcessBatchChunk(SendMsgBuffer_t* sendPlainChunkBuf,
            SendMsgBuffer_t* sendCipherChunkBuf, SendMsgBuffer_t* sendRecipeBuf);
        
        /**
         * @brief close the connection with key manager
         * 
         */
        void CloseKMConnection();
};

#endif