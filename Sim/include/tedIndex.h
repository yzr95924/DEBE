/**
 * @file tedIndex.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2021-12-25
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef TED_INDEX_H
#define TED_INDEX_H

#include "define.h"
#include "absIndex.h"
#include "cmSketch.h"
#include "absDatabase.h"
#include "optSolver.h"
#include "randomGen.h"
#include "cryptoPrimitive.h"

class TEDIndex : public AbsIndex {
    private:
        string myName_ = "TED";
        CountMinSketch* countMinSketch_;

        double storageBlowup_ = 1.15;
        uint32_t threshold_ = 0;

        uint32_t width_ = (1 << 20);
        uint32_t depth_ = 4;

        RandomGen randomNumGen_;

        CryptoPrimitive* cryptoObj_;
    public:

        /**
         * @brief Construct a new TEDIndex object
         * 
         * @param inputFile the trace file
         * @param type FSL or MS
         */
        TEDIndex(string inputFile, int type);

        /**
         * @brief process the trace file
         * 
         * @return true success
         * @return false fail 
         */
        bool ProcessTrace();

        /**
         * @brief Destroy the TEDIndex object
         * 
         */
        ~TEDIndex();
};


#endif