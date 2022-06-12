/// \file randomGen.h
/// \author Zuoru YANG (zryang@cse.cuhk.edu.hk)
/// \brief define the interface of random number generator
/// \version 0.1
/// \date 2019-06-30
///
/// \copyright Copyright (c) 2019
///

#ifndef __RANDOM_GEN_H__
#define __RANDOM_GEN_H__
#include <iostream>
#include <stdio.h>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <math.h>
#include <stdlib.h>

#define UNIFORM_DIS 1
#define POISSON_DIS 2
#define NORMAL_DIS 3
#define GEO_DIS 4

using namespace std;



class RandomGen {
    private:
        random_device rd_;  //Will be used to obtain a seed for the random number engine
        mt19937 gen_ = mt19937(rd_());
        /// \brief Get the Uniform Random object
        ///
        /// \param K - the current index
        /// \return int - generated random number 
        int inline GetUniformRandom(int K) {
            uniform_int_distribution<> dis(0, K);
            int result = dis(gen_);
            return result;
        }

        /// \brief Get the Poisson Random object
        ///
        /// \param K - the current index
        /// \return int - the generated random number 
        int inline GetPoissonRandom(int K) {
            int lambda = ceil(K / 2.0);
            poisson_distribution<> dis(lambda);
            int result = round(dis(gen_));
            return result;
        }
        
        /// \brief Get the Normal Random object
        ///
        /// \param K - the current index
        /// \return int - the generated random number
        int inline GetNormalRandom(int K) {
            double central = K / 2.0;
            normal_distribution<> dis(central, 1);
            int result = round(dis(gen_));
            if (result < 0) {
                return 0;
            } else {
                return result;
            }
        }

        /// \brief Get the Geo Random object
        ///
        /// \param K - the current index
        /// \return int - the generated random number
        int inline GetGeoRandom(int K) {
            geometric_distribution<> dis;
            int random = round(dis(gen_));
            if (K <= random) {
                return 0;
            } else {
                return (K - random);
            }
        }

    public:
        /// \brief Construct a new Random Gen object
        ///
        RandomGen();

        /// \brief Destroy the Random Gen object
        ///
        ~RandomGen() {};

        /// \brief produce a random number according to different types
        ///
        /// \param type - the type of random number probability distribution
        /// \param K - the current index
        /// \return int - the generated number
        int ProRandomNumber(int type, int K);


};

#endif // !__RANDOM_GEN_H__