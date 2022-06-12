/**
 * @file plainIndex.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of plain index
 * @version 0.1
 * @date 2021-05-14
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef PLAIN_INDEX_H
#define PLAIN_INDEX_H

#include "absIndex.h"
#include "clientVar.h"
#include "clientVar.h"
#include <lz4.h>

class PlainIndex : public AbsIndex {
    private:
        string myName_ = "DedupIndex";

    public:
        /**
         * @brief Construct a new Plain Index object
         * 
         * @param indexStore the reference to the index store
         */
        PlainIndex(AbsDatabase* indexStore);

        /**
         * @brief Destroy the Plain Index object destore the plain index
         * 
         */
        ~PlainIndex();

        /**
         * @brief process one batch 
         * 
         * @param recvChunkBuf the recv chunk buffer
         * @param curClient the current client var
         */
        void ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, ClientVar* curClient);

        /**
         * @brief process the tail segment
         * 
         * @param curClient the current client var
         */
        void ProcessTailBatch(ClientVar* curClient);
};

#endif