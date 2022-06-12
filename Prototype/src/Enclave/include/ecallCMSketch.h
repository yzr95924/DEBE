/**
 * @file ecallCMSketch.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of CM-Sketch inside the enclave
 * @version 0.1
 * @date 2021-01-13
 * 
 * @copyright Copyright (c) 2021
 * 
 */


#ifndef ECALL_CM_SKETCH_H
#define ECALL_CM_SKETCH_H

#include "commonEnclave.h"

#define LONG_PRIME 4294967311l

class EcallCMSketch {
    /**width, depth */
        uint32_t width_;
        uint32_t depth_;

        /**total count */
        size_t total_;

        /**array of arrays of counters */
        uint32_t** counterArray_;

        // hard code a seed for murmurhash
        // uint32_t constSeed_ = 1;

        // the buffer to store the hash result
        uint32_t* resultHash_;
    public:

        /**
         * @brief Construct a new Count Min Sketch object
         * 
         * @param width the width of the sketch
         * @param depth the depth of the sketch
         */
        EcallCMSketch(uint32_t width, uint32_t depth = 4);

        /**
         * @brief Update the sketch
         * 
         * @param chunkHash the input chunk hash
         * @param chunkHashLen the length of the chunk hash
         * @param count count number
         */
        void Update(const uint8_t* chunkHash, size_t chunkHashLen, uint32_t count);

        /**
         * @brief estimate the chunk count 
         * 
         * @param chunkHash the input chunk hash
         * @param chunkHashLen the length of the chunk hash
         * @return uint32_t count number
         */
        uint32_t Estimate(const uint8_t* chunkHash, size_t chunkHashLen);

        /**
         * @brief return the total number of processed items
         * 
         * @return size_t the total number of items
         */
        size_t TotalCount();

        /**
         * @brief Destroy the Count Min Sketch object
         * 
         */
        ~EcallCMSketch();

        /**
         * @brief clear the state of this sketch
         * 
         */
        void ClearUp();

        /**
         * @brief Get the Counter Array object
         * 
         * @return uint32_t** the pointer to the counter array
         */
        uint32_t** GetCounterArray() {
            return this->counterArray_;
        }
};

#endif