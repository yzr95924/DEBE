/**
 * @file absIndex.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of abstract index 
 * @version 0.1
 * @date 2020-10-14
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef ABS_INDEX_H 
#define ABS_INDEX_H

#include "define.h"
#include "absDatabase.h"
#include "factoryDatabase.h"
#include "cryptoPrimitive.h"

#define LOGICAL_CHUNK 0
#define CIPHER_CHUNK 1

#define INFO_FILE_NAME "stat.info" 

using namespace std;

enum Trace_t {FSL = 0, MS = 1}; // TODO: move this to the command

class AbsIndex {
    protected:
        string myName_ = "AbsIndex";
        string inputFile_;
        ifstream fpIn_;
        uint32_t chunkHashLen = 0;

        uint64_t logicalDataSize_ = 0;
        uint64_t uniqueDataSize_ = 0;
        uint64_t logicalChunkNum_ = 0;
        uint64_t uniqueChunkNum_ = 0;


        uint32_t k_ = 1;

        DatabaseFactory dbFactory_;

        // to record the frequency of each 
        AbsDatabase* logicalChunkDB_;
        AbsDatabase* cipherChunkDB_;

        // crypto tool 
        CryptoPrimitive* cryptoObj_;

        double originalKLD_ = 0;
        double newKLD_ = 0;

        /**
         * @brief count the chunk freq 
         * 
         * @param chunkFp the chunk fp
         * @param type logical chunk or cipher chunk
         */
        void CountChunkFreq(uint8_t* chunkFp, int type, int chunkSize);

        /**
         * @brief parse the chunk information according to the line 
         * 
         * @param readLineBuffer the line buffer
         * @param chunkFp chunk fingerprint
         * @return uint32_t chunk size
         */
        uint32_t ParseChunkInformation(char* readLineBuffer, uint8_t* chunkFp);

        /**
         * @brief compute the entropy
         * 
         */
        double ComputeEntropy(vector<uint32_t>& dist);

        /**
         * @brief compute the kld
         * 
         */
        void ComputeKLD();

        /**
         * @brief clear up for each run
         * 
         */
        void ClearUp();
    public:

        /**
         * @brief Construct a new Abs Index object
         * 
         * @param inputFile input trace file
         */
        AbsIndex(string inputFile, int type);

        /**
         * @brief process the trace file 
         * 
         * @return true success 
         * @return false fail  
         */
        virtual bool ProcessTrace() = 0;


        virtual ~AbsIndex() {
            if (fpIn_.is_open()) {
                fpIn_.close();
            } 
            delete cryptoObj_;
        };
};
#endif //  ABS_INDEX_H!