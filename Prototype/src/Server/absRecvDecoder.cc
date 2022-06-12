/**
 * @file absRecvDecoder.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of abs recv decoder
 * @version 0.1
 * @date 2021-02-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/absRecvDecoder.h"


/**
 * @brief Construct a new Abs Recv Decoder object
 * 
 * @param dataSecureChannel ssl connection pointer
 */
AbsRecvDecoder::AbsRecvDecoder(SSLConnection* dataSecureChannel) {
    // prepare the read recipe buffer
    sendChunkBatchSize_ = config.GetSendChunkBatchSize();
    sendRecipeBatchSize_ = config.GetSendRecipeBatchSize();

    // prepare the container cache
    containerNamePrefix_ = config.GetContainerRootPath();
    containerNameTail_ = config.GetContainerSuffix();

    // for communication 
    dataSecureChannel_ = dataSecureChannel;

    // for file recipe path
    recipeNamePrefix_ = config.GetRecipeRootPath();
    recipeNameTail_ = config.GetRecipeSuffix();

    tool::Logging(myName_.c_str(), "init the AbsRecvDecoder.\n");
}


/**
 * @brief Destroy the Abs Recv Decoder object
 * 
 */
AbsRecvDecoder::~AbsRecvDecoder() {
    
}