/**
 * @file tedIndex.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2021-12-25
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/tedIndex.h"

/**
 * @brief Construct a new TEDIndex object
 * 
 * @param inputFile the trace file
 * @param type FSL or MS
 */
TEDIndex::TEDIndex(string inputFile, int type) : 
    AbsIndex(inputFile, type) {
    countMinSketch_ = new CountMinSketch(width_, depth_);
    logicalChunkDB_ = dbFactory_.CreateDatabase(LEVEL_DB, "plaintext-db");
    cipherChunkDB_ = dbFactory_.CreateDatabase(LEVEL_DB, "ciphertext-db"); 
    cryptoObj_ = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);
    tool::Logging(myName_.c_str(), "init the TEDIndex.\n");
}

/**
 * @brief process the trace file
 * 
 * @return true success
 * @return false fail 
 */
bool TEDIndex::ProcessTrace() {
    tool::Logging(myName_.c_str(), "start to process %s\n", 
        inputFile_.c_str());
    string inputLine;
    EVP_CIPHER_CTX* cipherCtx = EVP_CIPHER_CTX_new();
    char readLineBuffer[256];

    while (getline(fpIn_, inputLine)) {
        // read trace file line by line
        memset(readLineBuffer, 0, 256);
        memcpy(readLineBuffer, inputLine.c_str(), inputLine.size());

        uint8_t chunkFp[chunkHashLen + 1];
        memset(chunkFp, 0, chunkHashLen + 1);

        uint64_t chunkSize = this->ParseChunkInformation(readLineBuffer,
            chunkFp); 
        
        if (chunkSize > MAX_CHUNK_SIZE) {
            chunkSize = MAX_CHUNK_SIZE;
        }

        // update the CM-sketch
        countMinSketch_->Update(chunkFp, chunkHashLen, 1);
    }

    tool::Logging(myName_.c_str(), "start to solve the optimization problem.\n");
    vector<pair<string, uint64_t>> inputDistri;
    uint32_t* sketchFirstRow = countMinSketch_->GetFirstRow();
    size_t index = 0;
    for (index = 0; index < width_; index++) {
        if (sketchFirstRow[index] != 0) {
            inputDistri.push_back(std::make_pair("1", 
                static_cast<uint64_t>(sketchFirstRow[index])));
        } 
    }

    // initial optimization solver
    OpSolver* mySolver = new OpSolver(storageBlowup_, inputDistri);
    threshold_ = mySolver->GetOptimal();
    delete mySolver;

    tool::Logging(myName_.c_str(), "start to clean up the sketch.\n");
    countMinSketch_->ClearUp();

    /****************Second pass *************
    * for calculation the optimization problem
    * **************************************
    */
    fpIn_.clear();
    fpIn_.seekg(0, ios_base::beg);
    while (getline(fpIn_, inputLine)) {
        memset(readLineBuffer, 0, 256);
        memcpy(readLineBuffer, inputLine.c_str(), inputLine.size());

        uint8_t chunkFp[chunkHashLen + 1];
        memset(chunkFp, 0, chunkHashLen + 1);

        uint64_t chunkSize = this->ParseChunkInformation(readLineBuffer,
            chunkFp); 
        
        if (chunkSize > MAX_CHUNK_SIZE) {
            chunkSize = MAX_CHUNK_SIZE;
        }

        this->CountChunkFreq(chunkFp, LOGICAL_CHUNK, chunkSize);
        logicalChunkNum_++;
        logicalDataSize_ += chunkSize;

        countMinSketch_->Update(chunkFp, chunkHashLen, 1);

        uint32_t frequency = countMinSketch_->Estimate(chunkFp, chunkHashLen);
        int state = frequency / (threshold_ + 0.00000001);
        state = randomNumGen_.ProRandomNumber(UNIFORM_DIS, state);
        uint8_t key[16] = {0};
        memcpy(key, &state, sizeof(state));
        uint8_t cipherFp[chunkHashLen];
        cryptoObj_->EncryptWithKey(cipherCtx, chunkFp, chunkHashLen,
            key, cipherFp);
        
        this->CountChunkFreq(cipherFp, CIPHER_CHUNK, 1);
    }

    EVP_CIPHER_CTX_free(cipherCtx);
    this->ComputeKLD();
    this->ClearUp();
    return true;
}

/**
 * @brief Destroy the TEDIndex object
 * 
 */
TEDIndex::~TEDIndex() {
    delete logicalChunkDB_;
    delete cipherChunkDB_;
    delete countMinSketch_;
    delete cryptoObj_;
    fprintf(stderr, "========TEDIndex Info========\n");
    fprintf(stderr, "OriginalKLD (CE): %lf\n", originalKLD_);
    fprintf(stderr, "Cipher KLD (TED): %lf\n", newKLD_);
    fprintf(stderr, "============================\n");
    // fprintf(stdout, "Logical chunk num: %lu\n", logicalChunkNum_);
    // fprintf(stdout, "Logical data size: %lu\n", logicalDataSize_);
    // fprintf(stdout, "Unique chunk num: %lu\n", uniqueChunkNum_);
    // fprintf(stdout, "Unique data size: %lu\n", uniqueDataSize_);
}