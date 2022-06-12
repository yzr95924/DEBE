/// \file optSolver.cc
/// \author Zuoru YANG (zryang@cse.cuhk.edu.hk)
/// \brief implement the interface defined in Optimization solver
/// \version 0.1
/// \date 2019-07-12
///
/// \copyright Copyright (c) 2019
///

#include "../../include/optSolver.h"

/// \brief Construct a new Op Solver object
///
/// \param storageBlowUp - storage blowup rate
/// \param inputDistri 

inline double Log2(double number) {
    return log(number) / log(2);
}


OpSolver::OpSolver(double blowUpRate, vector<pair<string, uint64_t> > inputDistri) {

    /**Initialization */
    storageBlowUp_ = blowUpRate;
    tool::Logging(myName_.c_str(), "Storage Blowup: %lf\n", storageBlowUp_);
    uniqueChunkNum_ = inputDistri.size();

    inputFeqDistr_ = inputDistri;

    totalChunkNum_ = 0;
    for (auto iter = inputFeqDistr_.begin(); iter != inputFeqDistr_.end(); iter++) {
        totalChunkNum_ += iter->second;
    }

    remainSum_ = totalChunkNum_;

    /**comput the unique chunk after TEC */
    expUniqueChunkNum_ = static_cast<size_t>(ceil(storageBlowUp_ * uniqueChunkNum_));

    // fprintf(stderr, "uniqueChunkNum_: %d\n", static_cast<int>(uniqueChunkNum_));
    // fprintf(stderr, "expUniqueChunkNum_: %lu\n", expUniqueChunkNum_);
    
    double expAverageFreq = static_cast<double>(totalChunkNum_) / (expUniqueChunkNum_);

    size_t index;
    for (index = 0; index < expUniqueChunkNum_; index++) {
        outputFeqDistr_.push_back(expAverageFreq);
    }

    /**calculate the entropy in this region */
    // double freq = 0;
    // for (auto iter = inputFeqDistr_.begin(); iter != inputFeqDistr_.end(); iter++) {
    //     freq = static_cast<double> (iter->second) / totalChunkNum_; 
    //     originalEntropy_ -= freq * Log2(freq); 
    // }
    // fprintf(stderr, "Original entropy: %lf\n", originalEntropy_);
}

uint32_t OpSolver::GetOptimal() {
    sort(inputFeqDistr_.begin(), inputFeqDistr_.end(), [=](pair<string, int> a, pair<string, int> b)
    { return a.second < b.second;});

    /**initialize */
    size_t finishItem = 0;
    currentAvg_ = 0;
    size_t startIndex = 0;
    
    /**start to solve the optimization problem */
    while (true) {
        if (CheckConstrain(startIndex)) {
            if (currentAvg_ < 1) {
                currentAvg_ = 1;
            }
            break;
        } else {
            outputFeqDistr_[currentIndex_] = inputFeqDistr_[currentIndex_].second;
            finishItem++;
            remainSum_ -= outputFeqDistr_[currentIndex_];
            currentAvg_ = static_cast<double>(remainSum_) / (expUniqueChunkNum_ - finishItem);
            startIndex = currentIndex_ + 1;
            outputFeqDistr_[startIndex] = currentAvg_;    
        }
    }
    finalResult_ = static_cast<uint32_t>(ceil(currentAvg_));
    return finalResult_;
}

/// \brief check wether result can satisfy the all constrains
///
/// \param startIndex 
/// \return true 
/// \return false 
inline bool OpSolver::CheckConstrain(size_t startIndex) {
    int flag = 1;
    size_t counter = startIndex;
    while (counter < uniqueChunkNum_) {
        if (inputFeqDistr_[counter].second < outputFeqDistr_[counter]){
            flag = 0;
            /**store the current counter*/
            currentIndex_ = counter;
            break;    
        }
        counter++;
    }

    if (flag) {
        return true;
    } else {
        return false;
    }
}

/// \brief show the result of solving optimization problem
///
void OpSolver::PrintResult() {
    fprintf(stderr, "the number of unique chunk in this region: %lu\n", 
        uniqueChunkNum_);
    fprintf(stderr, "the number of expected unique chunk: %lu\n", 
        expUniqueChunkNum_);
    fprintf(stderr, "current average: %.4lf\n", currentAvg_);
    fprintf(stderr, "current Index: %lu, Value: %lu, Pass Rate: %.4lf\n",
        currentIndex_, inputFeqDistr_[currentIndex_].second,  
        static_cast<double>(currentIndex_) / (uniqueChunkNum_));
    fprintf(stderr, "the final result: %u\n", finalResult_);
}
