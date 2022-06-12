/**
 * @file absIndex.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in absIndex 
 * @version 0.1
 * @date 2020-10-14
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/absIndex.h"
#define DELIM "\t\n: "

/**
 * @brief Construct a new Abs Index object
 * 
 * @param inputFile input trace file
 */
AbsIndex::AbsIndex(string inputFile, int type) {
    inputFile_ = inputFile;
    fpIn_.open(inputFile_, ios_base::in);
    if (!fpIn_.is_open()) {
        tool::Logging(myName_.c_str(), "cannot open the trace file.\n");
        exit(EXIT_FAILURE);
    }
    cryptoObj_ = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);
    switch (type) {
        case FSL: {
            chunkHashLen = 6;
            break;
        }
        case MS: {
            chunkHashLen = 5;
            break;
        }
    }
}


/**
 * @brief parse the chunk information according to the line 
 * 
 * @param chunkFp chunk fingerprint
 * @return uint32_t chunk size
 */
uint32_t AbsIndex::ParseChunkInformation(char* readLineBuffer, uint8_t* chunkFp) {
    char* item; 
    item = strtok(readLineBuffer, DELIM);
    for (size_t i = 0; i < chunkHashLen; i++) {
        chunkFp[i] = strtol(item, NULL, 16); 
        item = strtok(NULL, DELIM);
    }
    chunkFp[chunkHashLen] = '\0';

    uint32_t chunkSize = atoi(item);
    return chunkSize;
}

/**
 * @brief clear up for each run
 * 
 */
void AbsIndex::ClearUp() {
    // size_t heapSize = newMinHeap_->Size();
    // fprintf(stdout, "The number of entry: %lu\n", heapSize);
    // for (size_t i = 0; i < heapSize; i++) {
    //     cout << newMinHeap_->TopEntry() << endl;
    //     if (newMinHeap_->Size() > 1) {
    //         newMinHeap_->Pop();
    //     } else {
    //         cout << newMinHeap_->TopEntry() << endl;
    //         break;
    //     }
    // }
    
    // reset
    if (newKLD_ > static_cast<double>(1 << 2)) {
        newKLD_ -= k_/ static_cast<double>(1 << 20);
    }
    return ;
}

/**
* @brief count the chunk freq 
* 
* @param chunkFp the chunk fp
* @param type logical chunk or cipher chunk
*/
void AbsIndex::CountChunkFreq(uint8_t* chunkFp, int type, int chunkSize) {
    string key;
    key.assign((const char*) chunkFp, chunkHashLen);

    string freqStr;
    freqStr.resize(sizeof(uint32_t), 0);

    bool findResult;
    if (type == LOGICAL_CHUNK) {
        findResult = logicalChunkDB_->Query(key, freqStr);
        if (findResult == false) {
            // this chunk is an unique chunk
            uint32_t freq = 1;
            memcpy(&freqStr[0], &freq, sizeof(uint32_t));
            logicalChunkDB_->Insert(key, freqStr);
        } else {
            // this chunk is a duplicate chunk
            uint32_t freq;
            memcpy(&freq, &freqStr[0], sizeof(uint32_t));
            freq++;
            memcpy(&freqStr[0], &freq, sizeof(uint32_t));
            logicalChunkDB_->Insert(key, freqStr);
        }

    } else if (type == CIPHER_CHUNK) {
        findResult = cipherChunkDB_->Query(key, freqStr);
        if (findResult == false) {
            // this chunk is an unique chunk
            uint32_t freq = 1;
            memcpy(&freqStr[0], &freq, sizeof(uint32_t));
            cipherChunkDB_->Insert(key, freqStr);
        } else {
            // this chunk is a duplicate chunk
            uint32_t freq;
            memcpy(&freq, &freqStr[0], sizeof(uint32_t));
            freq++;
            memcpy(&freqStr[0], &freq, sizeof(uint32_t));
            cipherChunkDB_->Insert(key, freqStr);
        }
    } else {
        tool::Logging(myName_.c_str(), "wrong db type.\n");
        exit(EXIT_FAILURE);
    }

    return ;
}

/**
 * @brief compute the entropy
 * 
 */
double AbsIndex::ComputeEntropy(vector<uint32_t>& dist) {
    uint64_t sum = 0;
    for (auto it : dist) {
        sum += it;
    }
    double entropy = 0;
    for (auto it :dist) {
        double freq = (double)(it) / sum;
        entropy = entropy - (freq * (log2(freq)));
    }
    return entropy;
}

/**
 * @brief compute the kld
 * 
 */
void AbsIndex::ComputeKLD() {
    // comput the original KLD
    vector<uint32_t> plainDist;
    logicalChunkDB_->GetValueUint32(plainDist);
    vector<uint32_t> cipherDist;
    cipherChunkDB_->GetValueUint32(cipherDist);

    originalKLD_ = log2(plainDist.size()) - this->ComputeEntropy(plainDist);
    newKLD_ = log2(cipherDist.size()) - this->ComputeEntropy(cipherDist);
    return ;
}