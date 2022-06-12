/// \file countMin.cc
/// \author Zuoru YANG (zryang@cse.cuhk.edu.hk)
/// \brief implement the interfaces of CountMin Sketch
/// \version 0.1
/// \date 2019-08-08
///
/// \copyright Copyright (c) 2019
///

#include "../../include/cmSketch.h"

/// \brief Construct a new Count Min Sketch object
///
/// \param width 
/// \param depth 
CountMinSketch::CountMinSketch(uint32_t width, uint32_t depth) {
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
    hashArray_ = (uint32_t*) malloc(sizeof(uint32_t) * depth_);

    for (i = 0; i < depth_; i++) {
        // seed 0, 1, 2, 3
        hashArray_[i] = i;
    }

    resultHash_ = (uint8_t*) malloc(sizeof(uint8_t) * SHORT_HASH_LEN);
}

/// \brief estimate the count in sketch
///
/// \param chunkHash 
/// \param chunkHashLen 
/// \return uint32_t 
uint32_t CountMinSketch::Estimate(uint8_t* const chunkHash, size_t chunkHashLen) {
    uint32_t minVal = UINT32_MAX; // max uint32_t value
    uint32_t hashVal = 0;
    size_t j = 0;
    size_t pos = 0;
    for (j = 0; j < depth_; j++) {
        MurmurHash3_x86_32(chunkHash, chunkHashLen, hashArray_[j], &hashVal);
        // cout << "hashValue: " << hashVal << endl;
        pos = hashVal % width_;
        minVal = min(minVal, counterArray_[j][pos]);
    }
    return minVal;
}


/// \brief Destroy the Count Min Sketch object
///
CountMinSketch::~CountMinSketch() {
    size_t index;
    
    if (hashArray_) {
        free(hashArray_);
    }

    for (index = 0; index < depth_; index++) {
        free(counterArray_[index]);
    }
    free(counterArray_);

    if (resultHash_) {
        free(resultHash_);
    }
}

/// \brief return total count
///
/// \return uint32_t 
uint32_t CountMinSketch::TotalCount() {
    return total_;
}

/// \brief update the count in sketch 
///
/// \param chunkHash 
/// \param chunkHashLen 
/// \param count 
void CountMinSketch::Update(uint8_t* const chunkHash, size_t chunkHashLen, uint32_t count) {
    total_ += count;
    uint32_t hashVal = 0;
    size_t j = 0;
    size_t pos = 0;
    for (j = 0; j < depth_; j++) {
       MurmurHash3_x86_32(chunkHash, chunkHashLen, hashArray_[j], &hashVal);
       pos = hashVal % width_;
       counterArray_[j][pos] += count;
    }
}


/// \brief clear all buckets in the sketch
///
void CountMinSketch::ClearUp() {
    size_t indexWidth;
    size_t indexDepth;
    for (indexWidth = 0; indexWidth < width_; indexWidth++) {
        for (indexDepth = 0; indexDepth < depth_; indexDepth++) {
            counterArray_[indexDepth][indexWidth] = 0;
        }
    }
}

/// \brief return the pos of first row
///
/// \param chunkHash 
/// \param chunkHashLen
/// \return uint32_t - the position 
uint32_t CountMinSketch::ReturnFirstRowPos(uint8_t* const chunkHash, size_t chunkHashLen) {
    uint32_t hashVal = 0;
    MurmurHash3_x86_32(chunkHash, chunkHashLen, hashArray_[0], &hashVal);
    uint32_t pos = hashVal % width_;
    return pos;
}