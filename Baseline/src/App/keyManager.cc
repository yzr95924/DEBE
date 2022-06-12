/**
 * @file keyManager.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the main process of the key manager
 * @version 0.1
 * @date 2021-12-12
 * 
 * @copyright Copyright (c) 2021
 * 
 */

// for the key manager
#include "../../include/absKM.h"
#include "../../include/tedKM.h"
#include "../../include/dupLESSKM.h"

// to receive the interrupt
#include <signal.h>
#include <boost/thread/thread.hpp>

using namespace std;

Configure config("config.json");

SSLConnection* keyManagerChannel;
vector<boost::thread*>  thList;

// the key manager main thread
AbsKeyManager* absKMObj;

string myName = "KeyManager";

void Usage() {
    fprintf(stderr, "./KeyManager -m [type].\n"
        "-m: method type ([type]):\n"
        "\t2: TED\n"
        "\t3: DupLESS\n");
    return ;
}

void CTRLC(int s) {
    tool::Logging(myName.c_str(), "terminate the key manager with ctrl+c interrupt.\n");
    // ------ clean up ------
    for (auto it : thList) {
        it->join();
    }

    for (auto it : thList) {
        delete it;
    }

    delete absKMObj;
    tool::Logging(myName.c_str(), "clear all key manager threads.\n");

    delete keyManagerChannel;
    tool::Logging(myName.c_str(), "clear network connection.\n");
    exit(EXIT_SUCCESS);
}


int main(int argc, char* argv[]) {
 
    // ------- main process --------

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
    uint32_t keyGenType;
    while ((option = getopt(argc, argv, optString)) != -1) {
        switch (option) {
            case 'm': {
                switch (atoi(optarg)) {
                    case TED_DAE: {
                        keyGenType = TED_DAE;
                        break;
                    }
                    case DUPLESS_DAE: {
                        keyGenType = DUPLESS_DAE;
                        break; 
                    }
                    default: {
                        tool::Logging(myName.c_str(), "wrong key gen type.\n");
                        Usage();
                        exit(EXIT_FAILURE);
                    }
                }
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

    keyManagerChannel = new SSLConnection(config.GetKeyServerIP(), config.GetKeyServerPort(), 
        IN_SERVERSIDE);
    
    // init
    switch (keyGenType) {
        case TED_DAE: {
            absKMObj = new TEDKeyManager(keyManagerChannel);
            break;
        }
        case DUPLESS_DAE: {
            absKMObj = new DupLESSKeyManager(keyManagerChannel);
            break;
        }
    }

    /**
     * |---------------------------------------|
     * |Finish the initialization of the server|
     * |---------------------------------------|
     */

    while (true) {
        tool::Logging(myName.c_str(), "waiting the request from the client.\n");
        SSL* keyClientSSL = keyManagerChannel->ListenSSL().second;
        thTmp = new boost::thread(attrs, boost::bind(&AbsKeyManager::Run, absKMObj, 
            keyClientSSL));
        thList.push_back(thTmp);
    }

    return 0;
}