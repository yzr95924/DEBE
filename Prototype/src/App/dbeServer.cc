/**
 * @file dbeServer.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the main server process
 * @version 0.1
 * @date 2021-01-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */
// for basic build block
#include "../../include/configure.h"
#include "../../include/clientVar.h"
#include "../../include/factoryDatabase.h"
#include "../../include/absDatabase.h"

// for main server thread
#include "../../include/serverOptThead.h"

// to receive the interrupt
#include <signal.h>
#include <boost/thread/thread.hpp>

// for SGX related
#include "sgx_urts.h"
#include "sgx_capable.h"
#include "../src/Enclave/include/storeOCall.h"

using namespace std;

// the variable to record the enclave information
sgx_enclave_id_t eidSGX;
sgx_launch_token_t tokenSGX = {0};
sgx_status_t statusSGX;
int updateSGX;

Configure config("config.json");
string myName = "DEBEServer";

SSLConnection* dataSecurityChannelObj;
DatabaseFactory dbFactory;
AbsDatabase* fp2ChunkDB;
vector<boost::thread*> thList;

ServerOptThread* serverThreadObj;

void Usage() {
    fprintf(stderr, "./DEBEServer -m [IndexType]\n"
        "-m: index type ([IndexType]):\n"
        "\t0: Out-Enclave Index\n"
        "\t1: In-Enclave Index\n"
        "\t2: Similarity-based Index\n"
        "\t3: Locality-based Index\n"
        "\t4: Freq-based Index\n");
    return ;
}

void CTRLC(int s) {
    tool::Logging(myName.c_str(), "terminate the server with ctrl+c interrupt\n");
    // ------ clean up ------
    for (auto it : thList) {
        it->join();
    }

    for (auto it : thList) {
        delete it;
    }

    delete serverThreadObj;
    tool::Logging(myName.c_str(), "clear all server thread the object.\n");

    // destroy the sgx here
    Ecall_Enclave_Destroy(eidSGX); 

    delete fp2ChunkDB;
    delete dataSecurityChannelObj;

    tool::Logging(myName.c_str(), "close all DBs and network connection.\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {

    // ------ main process ------

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sigIntHandler, 0);

    sigIntHandler.sa_handler = CTRLC;
    sigaction(SIGKILL, &sigIntHandler, 0);
    sigaction(SIGINT, &sigIntHandler, 0);

    srand(tool::GetStrongSeed());

    const char optString[] = "m:";
    int option;

    if (argc < sizeof(optString)) {
        tool::Logging(myName.c_str(), "wrong argc: %d\n", argc);
        Usage();
        exit(EXIT_FAILURE);
    }

    // parse the arg
    int indexType;
    while ((option = getopt(argc, argv, optString)) != -1) {
        switch (option) {
            case 'm': {
                indexType = atoi(optarg);
                break;
            }
            case '?': {
                tool::Logging(myName.c_str(), "error optopt: %c\n", optopt);
                tool::Logging(myName.c_str(), "error opterr: %d\n", opterr);
                Usage();
                exit(EXIT_FAILURE);
            }
        }
    }

    boost::thread* thTmp;
    boost::thread_attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);
    
    fp2ChunkDB = dbFactory.CreateDatabase(IN_MEMORY, config.GetFp2ChunkDBName());
    dataSecurityChannelObj = new SSLConnection(config.GetStorageServerIP(), 
        config.GetStoragePort(), IN_SERVERSIDE);

    // check whether enable SGX
#if (CHECK_SGX_HW == 1)
    updateSGX = 0;
    sgx_is_capable(&updateSGX);
    if (updateSGX != 1) {
        tool::Logging(myName.c_str(), "SGX is disabled on this PC.\n");
        exit(EXIT_FAILURE);
    } else {
        tool::Logging(myName.c_str(), "SGX is enable.\n");
    }
#endif
    statusSGX = sgx_create_enclave(ENCLAVE_PATH, SGX_DEBUG_FLAG, &tokenSGX,
        &updateSGX, &eidSGX, NULL);
    if (statusSGX != SGX_SUCCESS) {
        tool::Logging(myName.c_str(), "fail to create the enclave.\n");
        exit(EXIT_FAILURE);
    } else {
        tool::Logging(myName.c_str(), "create the enclave successfully.\n");
    }

    // config the enclave
    EnclaveConfig_t enclaveConfig;
    enclaveConfig.sendChunkBatchSize = config.GetSendChunkBatchSize();
    enclaveConfig.sendRecipeBatchSize = config.GetSendRecipeBatchSize();
    enclaveConfig.topKParam = config.GetTopKParam();
    Ecall_Enclave_Init(eidSGX, &enclaveConfig);

    // init 
    serverThreadObj = new ServerOptThread(dataSecurityChannelObj, fp2ChunkDB,
        eidSGX, indexType);

    /**
     * |---------------------------------------|
     * |Finish the initialization of the server|
     * |---------------------------------------|
     */

    while (true) {
        tool::Logging(myName.c_str(), "waiting the request from the client.\n");
        SSL* clientSSL = dataSecurityChannelObj->ListenSSL().second;
        thTmp = new boost::thread(attrs, boost::bind(&ServerOptThread::Run, serverThreadObj,
            clientSSL));
        thList.push_back(thTmp);
    }

    return 0;
}