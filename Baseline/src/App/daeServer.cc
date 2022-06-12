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
#include "../../include/serverOptThread.h"

// to receive the interrupt
#include <signal.h>
#include <boost/thread/thread.hpp>

using namespace std;

Configure config("config.json");
string myName = "DAEServer";

SSLConnection* serverChannelObj;
DatabaseFactory dbFactory;
AbsDatabase* fp2ChunkDB;
vector<boost::thread*> thList;

ServerOptThread* serverThreadObj;

void Usage() {
    fprintf(stderr, "./DAEServer\n");
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

    delete fp2ChunkDB;
    delete serverChannelObj;

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

    boost::thread* thTmp;
    boost::thread_attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);
    
    fp2ChunkDB = dbFactory.CreateDatabase(IN_MEMORY, config.GetFp2ChunkDBName());
    serverChannelObj = new SSLConnection(config.GetStorageServerIP(), 
        config.GetStoragePort(), IN_SERVERSIDE);

    // init
    int indexType = 0;
    serverThreadObj = new ServerOptThread(serverChannelObj, fp2ChunkDB, indexType);

    /**
     * |---------------------------------------|
     * |Finish the initialization of the server|
     * |---------------------------------------|
     */

    while (true) {
        tool::Logging(myName.c_str(), "waiting the request from the client.\n");
        SSL* clientSSL = serverChannelObj->ListenSSL().second;
        thTmp = new boost::thread(attrs, boost::bind(&ServerOptThread::Run, serverThreadObj,
            clientSSL));
        thList.push_back(thTmp);
    }

    return 0;
}