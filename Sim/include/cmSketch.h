/// \file countMin.h
/// \author Zuoru YANG (zryang@cse.cuhk.edu.hk)
/// \brief define the interface of Count-Min sketch 
/// \version 0.1
/// \date 2019-08-08
///
/// \copyright Copyright (c) 2019
///
#ifndef __COUNT_MIN_H__
#define __COUNT_MIN_H__

#include "define.h"
#include "murmurHash3.h"

#define LONG_PRIME 4294967311l
#define SHORT_HASH_LEN 16

using namespace std;

class CountMinSketch {
    private:
        /**width, depth */
        uint32_t width_;
        uint32_t depth_;


        /**total count */
        uint32_t total_;

        /**array of arrays of counters */
        uint32_t** counterArray_;

        /**array of hash values for a particular item
         * contains two element arrays {aj, bj}
         */
        uint32_t* hashArray_;


        uint8_t* resultHash_;

    public:

        /// \brief return the pos of first row
        ///
        /// \param chunkHash 
        /// \param chunkHashLen 
        /// \return uint32_t - the position 
        uint32_t ReturnFirstRowPos(uint8_t* const chunkHash, size_t chunkHashLen);

        /// \brief Construct a new Count Min Sketch object
        ///
        /// \param width 
        /// \param depth 
        CountMinSketch(uint32_t width, uint32_t depth);

        /// \brief update the count in sketch 
        ///
        /// \param chunkHash 
        /// \param chunkHashLen 
        /// \param count 
        void Update(uint8_t* const chunkHash, size_t chunkHashLen, uint32_t count);

        /// \brief estimate the count in sketch
        ///
        /// \param chunkHash 
        /// \param chunkHashLen 
        /// \return uint32_t 
        uint32_t Estimate(uint8_t* const chunkHash, size_t chunkHashLen);

        /// \brief return total count
        ///
        /// \return uint32_t 
        uint32_t TotalCount();

        /// \brief Destroy the Count Min Sketch object
        ///
        ~CountMinSketch();

        /// \brief clear all buckets in the sketch
        ///
        void ClearUp();

        /// \brief Get the First Row of the sketch
        ///
        /// \return uint32_t* - the pointer points to the first row of sketch
        inline uint32_t* GetFirstRow() {
            return counterArray_[0];
        }

        uint32_t** GetCounterArray() {
            return this->counterArray_;
        }
};



#endif // !__COUNT_MIN_H__