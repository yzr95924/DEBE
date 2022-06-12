/**
 * @file dbeClient.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the main process of DBE client process
 * @version 0.1
 * @date 2021-01-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/configure.h"

// for upload operation
#include "../../include/chunker.h"
#include "../../include/dataSender.h"
#include "../../include/absDaE.h"

// for the DaE
#include "../../include/plainDaE.h"
#include "../../include/mleDaE.h"
#include "../../include/tedDaE.h"
#include "../../include/duplessDaE.h"

// for restore operation
#include "../../include/dataRetriever.h"
#include "../../include/restoreWriter.h"

#include <boost/thread/thread.hpp>

using namespace std;

struct timeval sTime;
struct timeval eTime;
double totalTime = 0;

Configure config("config.json");
string myName = "DAEClient";
string logFileName = "client-log";
ofstream logFile;

void Usage() {
    fprintf(stderr, "./DAEClient -t [u/d] -i [inputFile path] -m [DaEType].\n"
        "-t: operation ([u/d]):\n"
        "\tu: upload\n"
        "\td: download\n"
        "-m: DaE type ([DaEType]):\n"
        "\t0: Plain\n"
        "\t1: CE-DaE\n"
        "\t2: TED-DaE\n"
        "\t3: DupLESS-DaE\n");
    return ;
}

int main(int argc, char* argv[]) {
    // for log file
    if (!tool::FileExist(logFileName)) {
        // if the log file not exits, add the header
        logFile.open(logFileName, ios_base::out);
        logFile << "input file, " << "opt, "
            << "logical data size (B), " << "logical chunk num, "
            << "total time (s), " << "speed (MiB/s)" << endl;
    } else {
        // the log file exists
        logFile.open(logFileName, ios_base::app | ios_base::out);    
    }

    vector<boost::thread*> thList;

    // for server connection    
    SSLConnection* serverChannel;
    pair<int, SSL*> conChannelRecord;
    SSL* serverConnection;

    // for upload operation
    DataSender* dataSenderObj;
    Chunker* chunkerObj;
    AbsDAE* absDaEObj;
    CryptoPrimitive* cryptoObj;

    // for restore operation 
    DataRetriever* dataRetrieverObj;
    RestoreWriter* restoreWriterObj;

    // ------ main process ------

    const char optString[] = "t:i:m:";
    int option;

    if (argc < sizeof(optString)) {
        tool::Logging(myName.c_str(), "wrong argc: %d\n", argc);
        Usage();
        exit(EXIT_FAILURE);
    }

    uint32_t optType;
    string inputFile;
    int methodType; 
    while ((option = getopt(argc, argv, optString)) != -1) {
        switch (option) {
            case 't': {
                if (strcmp("u", optarg) == 0) {
                    optType = UPLOAD_OPT;
                    break;
                } else if (strcmp("d", optarg) == 0) {
                    optType = DOWNLOAD_OPT;
                    break;
                } else {
                    tool::Logging(myName.c_str(), "wrong client operation type.\n");
                    Usage();
                    exit(EXIT_FAILURE);
                }
            }
            case 'i': {
                inputFile.assign(optarg);
                break;
            }
            case 'm': {
                switch (atoi(optarg)) {
                    case PLAIN_DAE: {
                        methodType = PLAIN_DAE;
                        break;
                    }
                    case CE_DAE: {
                        methodType = CE_DAE;
                        break;
                    }
                    case TED_DAE: {
                        methodType = TED_DAE;
                        break;
                    }
                    case DUPLESS_DAE: {
                        methodType = DUPLESS_DAE;
                        break;
                    }
                    default: {
                        tool::Logging(myName.c_str(), "wrong method type.\n");
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
    boost::thread::attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);
    cryptoObj = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);
    EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();

    // connect to the storage server
    serverChannel = new SSLConnection(config.GetStorageServerIP(), config.GetStoragePort(),
        IN_CLIENTSIDE);
    conChannelRecord = serverChannel->ConnectSSL();
    serverConnection = conChannelRecord.second;
    uint32_t clientID = config.GetClientID();

    // compute the file name hash
    string fullName = inputFile + to_string(clientID);
    uint8_t fileNameHash[CHUNK_HASH_SIZE] = {0};
    cryptoObj->GenerateHash(mdCtx, (uint8_t*)&fullName[0], 
        fullName.size(), fileNameHash);
    
    switch (optType) {
        case UPLOAD_OPT: { // for upload operation
            tool::Logging(myName.c_str(), "upload input file name: %s\n", 
                inputFile.c_str());
            chunkerObj = new Chunker(inputFile);
            switch (methodType) {
                case PLAIN_DAE: {
                    absDaEObj = new PlainDAE(fileNameHash);
                    break;
                }
                case CE_DAE: {
                    absDaEObj = new MLEDAE(fileNameHash);
                    break;
                }
                case TED_DAE: {
                    absDaEObj = new TEDDAE(fileNameHash);
                    break;
                }
                case DUPLESS_DAE: {
                    absDaEObj = new DupLESSDAE(fileNameHash); 
                    break;
                }
            }
            dataSenderObj = new DataSender(serverChannel, absDaEObj);
            dataSenderObj->SetConnectionRecord(conChannelRecord);

            // prepare the MQ
            MessageQueue<Data_t>* chunker2SenderMQ = new MessageQueue<Data_t>(CHUNK_QUEUE_SIZE);
            chunkerObj->SetOutputMQ(chunker2SenderMQ);
            dataSenderObj->SetInputMQ(chunker2SenderMQ);

            dataSenderObj->UploadLogin(fileNameHash);

            thTmp = new boost::thread(attrs, boost::bind(&Chunker::Chunking, chunkerObj));
            thList.push_back(thTmp);
            thTmp = new boost::thread(attrs, boost::bind(&DataSender::Run, dataSenderObj));
            thList.push_back(thTmp);

            gettimeofday(&sTime, NULL);
            for (auto it : thList) {
                it->join();
            }
            gettimeofday(&eTime, NULL);
            totalTime += tool::GetTimeDiff(sTime, eTime);
            tool::Logging(myName.c_str(), "%s finish.\n", inputFile.c_str());

            for (auto it : thList) {
                delete it;
            }

            // update the log
            double speed = static_cast<double>(chunkerObj->_recipe.recipeHead.fileSize) / 
                1024.0 / 1024.0 / totalTime;
            logFile << inputFile << ", upload, "
                << chunkerObj->_recipe.recipeHead.fileSize << ", "
                << chunkerObj->_recipe.recipeHead.totalChunkNum << ", "
                << to_string(totalTime) <<  ", " 
                << to_string(speed) << endl;
            delete chunkerObj;
            delete absDaEObj;
            delete dataSenderObj;
            delete chunker2SenderMQ;
            thList.clear();
            break;
        }
        case DOWNLOAD_OPT: { // for download operation
            tool::Logging(myName.c_str(), "restore input file name: %s\n", 
                inputFile.c_str());
            restoreWriterObj = new RestoreWriter(inputFile);
            dataRetrieverObj = new DataRetriever(serverChannel, methodType, 
                fileNameHash);
            dataRetrieverObj->SetConnectionRecord(conChannelRecord);

            // prepare the MQ
            MessageQueue<Chunk_t>* retrieve2WriterMQ = new MessageQueue<Chunk_t>(CHUNK_QUEUE_SIZE);
            restoreWriterObj->SetInputMQ(retrieve2WriterMQ);
            dataRetrieverObj->SetOutputMQ(retrieve2WriterMQ);

            dataRetrieverObj->RestoreLogin(fileNameHash);

            thTmp = new boost::thread(attrs, boost::bind(&DataRetriever::Run, dataRetrieverObj));
            thList.push_back(thTmp);
            thTmp = new boost::thread(attrs, boost::bind(&RestoreWriter::Run, restoreWriterObj));
            thList.push_back(thTmp);

            gettimeofday(&sTime, NULL);
            for (auto it : thList) {
                it->join();
            }
            gettimeofday(&eTime, NULL);
            totalTime += tool::GetTimeDiff(sTime, eTime);
            tool::Logging(myName.c_str(), "%s finish.\n", inputFile.c_str());

            for (auto it : thList) {
                delete it;
            }

            // update the log
            double speed = static_cast<double>(dataRetrieverObj->_totalRecvDataSize) / 
                1024.0 / 1024.0 / totalTime;
            logFile << inputFile << ", download, "
                << dataRetrieverObj->_totalRecvDataSize << ", "
                << dataRetrieverObj->_totalRecvChunkNum << ", "
                << to_string(totalTime) << ", " 
                << to_string(speed) << endl;
            delete dataRetrieverObj;
            delete restoreWriterObj;
            delete retrieve2WriterMQ;
            thList.clear();
            break;
        }
    }

    EVP_MD_CTX_free(mdCtx);
    delete cryptoObj;
    delete serverChannel;
    tool::Logging(myName.c_str(), "total running time: %lf\n", totalTime);
    logFile.close();

    return 0;
}