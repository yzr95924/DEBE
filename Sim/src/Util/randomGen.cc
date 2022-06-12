/// \file randomGen.cc
/// \author Zuoru YANG (zryang@cse.cuhk.edu.hk)
/// \brief implement the interface define in random number generator
/// \version 0.1
/// \date 2019-06-30
///
/// \copyright Copyright (c) 2019
///

#include "../../include/randomGen.h"

/// \brief Construct a new Random Gen object
///
RandomGen::RandomGen() {

}


/// \brief produce a random number according to different types
///
/// \param type - the type of random number probability distribution
/// \param K - the current index
/// \return int - the generated number
int RandomGen::ProRandomNumber(int type, int K) {
    if (type == UNIFORM_DIS) {
        return GetUniformRandom(K);
    } else if (type == POISSON_DIS) {
        return GetPoissonRandom(K);
    } else if (type == NORMAL_DIS) {
        return GetNormalRandom(K);
    } else if (type == GEO_DIS) {
        return GetGeoRandom(K);
    } else {
        fprintf(stderr, "the random number type is wrong\n");
        exit(1);
    }
}