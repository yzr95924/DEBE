/**
 * @file storeOCall.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2022-04-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/storeOCall.h"

namespace OutEnclave {
    string myName_ = "OCall";
};

using namespace OutEnclave;

/**
 * @brief dump the inside container to the outside buffer
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_WriteContainer(void* outClient) {
    ClientVar* curClient = (ClientVar*)outClient;
    curClient->_inputMQ->Push(curClient->_curContainer);

    // reset current container
    tool::CreateUUID(curClient->_curContainer.containerID, 
        CONTAINER_ID_LENGTH);
    curClient->_curContainer.currentSize = 0;
    return ;
}