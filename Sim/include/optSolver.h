/// \file optSolver.h
/// \author Zuoru YANG (zryang@cse.cuhk.edu.hk)
/// \brief define the interface in optimization solver
/// \version 0.1
/// \date 2019-07-12
///
/// \copyright Copyright (c) 2019
///

#ifndef __OPT_SOLVER_H__
#define __OPT_SOLVER_H__

#include "define.h"


using namespace std;

class OpSolver {
    private:    
        string myName_ = "OptSolver";
        /**the degree of storage blowup rate */
        double storageBlowUp_;

        /**the number of total unique chunks */
        size_t uniqueChunkNum_;

        /**the number of total logical chunks */
        size_t totalChunkNum_;

        /**the number of expected unique chunks after the optimization */
        size_t expUniqueChunkNum_;
        
        /**record the current average*/
        double currentAvg_;

        /**the vector to store the input */
        vector<pair<string, uint64_t> > inputFeqDistr_;  

        /**the vector of ouput distribution */
        vector<double> outputFeqDistr_;    

        /// \brief check wether result can satisfy the all constrains
        ///
        /// \param startIndex 
        /// \return true - fulfill the constraint
        /// \return false - cannot fulfill the constraint
        bool CheckConstrain(size_t startIndex);

        /**current index */
        size_t currentIndex_ = 0;

        /**remian logical chunk*/
        size_t remainSum_;

        /**finResult of the threshold */
        uint32_t finalResult_ = 0;

    public:

        /// \brief Construct a new Op Solver object
        ///
        /// \param m - storage blowup rate
        /// \param inputDistri 
        OpSolver(double storageBlowUp, vector<pair<string, uint64_t> > inputDistri);

        /// \brief Destroy the Op Solver object
        ///
        ~OpSolver() { }


        /// \brief Get the Optimal object
        ///
        /// \return uint32_t 
        uint32_t GetOptimal();


        /// \brief show the result of solving optimization problem
        ///
        void PrintResult();        
};


#endif // !__OPT_SOLVER_H__