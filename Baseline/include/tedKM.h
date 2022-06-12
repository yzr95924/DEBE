/**
 * @file tedKM.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of ted key manager 
 * @version 0.1
 * @date 2021-12-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef TED_KM_H
#define TED_KM_H

#include "absKM.h"

class TEDKeyManager : public AbsKeyManager {
    private:
        string myName_ = "TEDKM";

        const uint64_t SKETCH_DEPTH = (1 << 2);
        const uint64_t SKETCH_WIDTH = (1 << 20);
        uint32_t** sketchTable_;
        random_device rd_; 
        mt19937_64 gen_; 
        uint64_t t_ = 5; // set the t = 5

    public:
        /**
         * @brief Construct a new TEDKeyManager object
         * 
         * @param keyManagerChannel the channel for key generation 
         */
        TEDKeyManager(SSLConnection* keyManagerChannel);

        /**
         * @brief Destroy the TEDKeyManager object
         * 
         */
        ~TEDKeyManager();

        /**
         * @brief the main thread
         * 
         * @param keyClientSSL the client ssl
         */
        void Run(SSL* keyClientSSL);
};

#endif