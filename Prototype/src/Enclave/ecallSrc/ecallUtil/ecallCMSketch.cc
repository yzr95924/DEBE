/**
 * @file ecallCMSketch.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in EcallCMSketch 
 * @version 0.1
 * @date 2021-01-13
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/ecallCMSketch.h"

/**
 * @brief Construct a new Count Min Sketch:: Count Min Sketch object
 * 
 * @param width the width of the sketch
 * @param depth the depth of the sketch
 */
EcallCMSketch::EcallCMSketch(uint32_t width, uint32_t depth) {
    width_ = width;
    depth_ = depth;
    total_ = 0;
    /**initialize counter array */
    counterArray_ = (uint32_t**) malloc(sizeof(uint32_t*) * depth_);
    size_t i, j;
    for (i = 0; i < depth_; i++) {
        counterArray_[i] = (uint32_t*) malloc(sizeof(uint32_t) * width_);
        for (j = 0; j < width_; j++) {
            counterArray_[i][j] = 0;
        }
    }

    /**initialize depth_ pairwise indepent hashes */
    resultHash_ = (uint32_t*) malloc(sizeof(uint32_t) * depth_);
}

/**
 * @brief estimate the chunk count 
 * 
 * @param chunkHash the input chunk hash
 * @param chunkHashLen the length of the chunk hash
 * @return uint32_t count number
 */
uint32_t EcallCMSketch::Estimate(const uint8_t* chunkHash, size_t chunkHashLen) {
    uint32_t minVal = UINT32_MAX; // max uint32_t value
    uint32_t pos;
    const uint32_t* chunkHashPtr = reinterpret_cast<const uint32_t*>(chunkHash);
    // MurmurHash3_x64_128(chunkHash, chunkHashLen, constSeed_, resultHash_);
    for (size_t i = 0; i < depth_; i++) {
        pos = chunkHashPtr[i] % width_;
        minVal = min(minVal, counterArray_[i][pos]);
    }
    return minVal;
}


/**
 * @brief Destroy the Count Min Sketch
 * 
 */
EcallCMSketch::~EcallCMSketch() {
    for (size_t index = 0; index < depth_; index++) {
        free(counterArray_[index]);
    }
    free(counterArray_);
    free(resultHash_);
}

/**
 * @brief return the total number of processed items
 * 
 * @return size_t the total number of items
 */
size_t EcallCMSketch::TotalCount() {
    return total_;
}

/**
 * @brief Update the sketch
 * 
 * @param chunkHash the input chunk hash
 * @param chunkHashLen the length of the chunk hash
 * @param count count number
 */
void EcallCMSketch::Update(const uint8_t* chunkHash, size_t chunkHashLen, uint32_t count) {
    total_ += count;
    uint32_t pos;
    const uint32_t* chunkHashPtr = reinterpret_cast<const uint32_t*>(chunkHash);
    // MurmurHash3_x64_128(chunkHash, chunkHashLen, constSeed_, resultHash_);
    for (size_t i = 0; i < depth_; i++) {
        pos = chunkHashPtr[i] % width_;
        counterArray_[i][pos] += count;
    }
    return ;
}

/**
 * @brief clear the state of this sketch
 * 
 */
void EcallCMSketch::ClearUp() {
    size_t indexWidth;
    size_t indexDepth;
    for (indexWidth = 0; indexWidth < width_; indexWidth++) {
        for (indexDepth = 0; indexDepth < depth_; indexDepth++) {
            counterArray_[indexDepth][indexWidth] = 0;
        }
    }
    return ;
}