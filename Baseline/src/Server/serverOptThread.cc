/**
 * @file serverOptThread.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the server main thread
 * @version 0.1
 * @date 2021-09-17
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/serverOptThread.h"

/**
 * @brief Construct a new Server Opt Thread object
 * 
 * @param serverChannel server communication channel
 * @param fp2ChunkDB the chunk info index
 * @param indexType the index type
 */
ServerOptThread::ServerOptThread(SSLConnection* serverChannel, AbsDatabase* fp2ChunkDB, 
    int indexType) { 
    serverChannel_ = serverChannel;
    fp2ChunkDB_ = fp2ChunkDB;
    indexType_ = indexType;

    // init the upload
    dataWriterObj_ = new DataWriter();
    storageCoreObj_ = new StorageCore();
    absIndexObj_ = new PlainIndex(fp2ChunkDB_);
    absIndexObj_->SetStorageCoreObj(storageCoreObj_);
    dataReceiverObj_ = new DataReceiver(absIndexObj_, serverChannel_);
    dataReceiverObj_->SetStorageCoreObj(storageCoreObj_);

    // init 
    recvDecoderObj_ = new RecvDecoder(serverChannel_);

    // for log file
    if (!tool::FileExist(logFileName_)) {
        // if the log file not exist, add the header
        logFile_.open(logFileName_, ios_base::out);
        logFile_ <<  "logical data size (B), " << "logical chunk num, "
            << "unique data size (B), " << "unique chunk num, "
            << "compressed data size (B), " << "total time (s)" << endl;
    } else {
        // the log file exists
        logFile_.open(logFileName_, ios_base::app | ios_base::out);
    }

    tool::Logging(myName_.c_str(), "init the ServerOptThread.\n");
}

/**
 * @brief Destroy the Server Opt Thread object
 * 
 */
ServerOptThread::~ServerOptThread() {
    delete dataWriterObj_;
    delete storageCoreObj_;
    delete absIndexObj_;
    delete dataReceiverObj_;
    delete recvDecoderObj_;

    for (auto it : clientLockIndex_) {
        delete it.second;
    }

    logFile_.close();

    fprintf(stderr, "========ServerOptThread Info========\n");
    fprintf(stderr, "total recv upload requests: %lu\n", totalUploadReqNum_);
    fprintf(stderr, "total recv download requests: %lu\n", totalRestoreReqNum_);
    fprintf(stderr, "====================================\n");
}

/**
 * @brief the main thread 
 * 
 * @param clientSSL the client ssl
 */
void ServerOptThread::Run(SSL* clientSSL) {
    boost::thread* thTmp;
    boost::thread_attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);
    vector<boost::thread*> thList;
    EnclaveInfo_t enclaveInfo;

    SendMsgBuffer_t recvBuf;
    recvBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + CHUNK_HASH_SIZE);
    recvBuf.header = (NetworkHead_t*) recvBuf.sendBuffer;
    recvBuf.header->dataSize = 0;
    recvBuf.dataBuffer = recvBuf.sendBuffer + sizeof(NetworkHead_t);
    uint32_t recvSize = 0;

    tool::Logging(myName_.c_str(), "the main thread is running.\n");

    // start to receive the data type
    if (!serverChannel_->ReceiveData(clientSSL, recvBuf.sendBuffer, 
        recvSize)) {
        tool::Logging(myName_.c_str(), "recv the login message error.\n");
        exit(EXIT_FAILURE);
    }

    // check the client lock here (ensure exist only one client with the same client ID)
    uint32_t clientID = recvBuf.header->clientID;
    boost::mutex* tmpLock;
    {
        // ensure only one client ID can enter the process
        lock_guard<mutex> lock(clientLockSetLock_);
        auto clientLockRes = clientLockIndex_.find(clientID);
        if (clientLockRes != clientLockIndex_.end()) {
            // try to lock this mutex
            tmpLock = clientLockRes->second;
            tmpLock->lock();
        } else {
            // add a new lock to the current index
            tmpLock = new boost::mutex();
            clientLockIndex_[clientID] = tmpLock;
            tmpLock->lock();
        }
    }

    struct timeval sTime;
    struct timeval eTime;

    // ---- the main process ----
    int optType = 0;
    switch (recvBuf.header->messageType) {
        case CLIENT_LOGIN_UPLOAD: {
            optType = UPLOAD_OPT;
            break;
        }
        case CLIENT_LOGIN_DOWNLOAD: {
            optType = DOWNLOAD_OPT;
            break;
        }
        default: {
            tool::Logging(myName_.c_str(), "wrong client login type.\n");
            exit(EXIT_FAILURE);
        }
    }

    // check the file status
    // convert the file name hash to the file path
    char fileHashBuf[CHUNK_HASH_SIZE * 2 + 1];
    for (int i = 0; i < CHUNK_HASH_SIZE; i++) {
        sprintf(fileHashBuf + i * 2, "%02x", recvBuf.dataBuffer[i]);
    }
    string fileName;
    fileName.assign(fileHashBuf, CHUNK_HASH_SIZE * 2); 
    string recipePath = config.GetRecipeRootPath() +
        fileName + config.GetRecipeSuffix();
    if (!this->CheckFileStatus(recipePath, optType)) {
        recvBuf.header->messageType = SERVER_FILE_NON_EXIST;
        if (!serverChannel_->SendData(clientSSL, recvBuf.sendBuffer,
            sizeof(NetworkHead_t))) {
            tool::Logging(myName_.c_str(), "send the file reply error.\n");
            exit(EXIT_FAILURE);
        }

        // wait the client to close the connection
        if (!serverChannel_->ReceiveData(clientSSL,
            recvBuf.sendBuffer, recvSize)) {
            tool::Logging(myName_.c_str(), "client close the socket connection.\n");
            serverChannel_->ClearAcceptedClientSd(clientSSL);
        } else {
            tool::Logging(myName_.c_str(), "client does not close the connection.\n");
            exit(EXIT_FAILURE);
        }
        
        // clear the tmp variable
        free(recvBuf.sendBuffer);
        tmpLock->unlock();
        return ;
    } else {
        tool::Logging(myName_.c_str(), "file status check successfully.\n");
    }
    /// check done

    // init the vars for this client
    ClientVar* curClient;
    switch (optType) {
        case UPLOAD_OPT: {
            // update the req number
            totalUploadReqNum_++;
            tool::Logging(myName_.c_str(), "recv the upload request from client: %u\n",
                clientID);
            curClient = new ClientVar(clientID, clientSSL, UPLOAD_OPT, recipePath);

            thTmp = new boost::thread(attrs, boost::bind(&DataReceiver::Run, dataReceiverObj_,
                curClient, &enclaveInfo));
            thList.push_back(thTmp); 
            thTmp = new boost::thread(attrs, boost::bind(&DataWriter::Run, dataWriterObj_,
                curClient->_inputMQ));
            thList.push_back(thTmp);

            // send the upload-response to the client
            recvBuf.header->messageType = SERVER_LOGIN_RESPONSE;
            if (!serverChannel_->SendData(clientSSL, recvBuf.sendBuffer, 
                sizeof(NetworkHead_t))) {
                tool::Logging(myName_.c_str(), "send the upload-login response error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case DOWNLOAD_OPT: {
            // update the req number 
            totalRestoreReqNum_++;
            tool::Logging(myName_.c_str(), "recv the restore request from client: %u\n",
                clientID);
            curClient = new ClientVar(clientID, clientSSL, DOWNLOAD_OPT, recipePath);

            thTmp = new boost::thread(attrs, boost::bind(&RecvDecoder::Run, recvDecoderObj_,
                curClient));
            thList.push_back(thTmp);

            // send the restore-reponse to the client (include the file recipe header)
            recvBuf.header->messageType = SERVER_LOGIN_RESPONSE;
            curClient->_recipeReadHandler.read((char*)recvBuf.dataBuffer,
                sizeof(FileRecipeHead_t));
            if (!serverChannel_->SendData(clientSSL, recvBuf.sendBuffer, 
                sizeof(NetworkHead_t) + sizeof(FileRecipeHead_t))) {
                tool::Logging(myName_.c_str(), "send the restore-login response error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        default: {
            tool::Logging(myName_.c_str(), "wrong operation type from client: %u\n",
                clientID);
            exit(EXIT_FAILURE);
        }
    }

    double totalTime = 0;
    gettimeofday(&sTime, NULL);
    for (auto it : thList) {
        it->join();
    }
    gettimeofday(&eTime, NULL);
    totalTime += tool::GetTimeDiff(sTime, eTime);

    // print the info
    if (optType == UPLOAD_OPT) {
        logFile_ << enclaveInfo.logicalDataSize << ", "
            << enclaveInfo.logicalChunkNum << ", "
            << enclaveInfo.uniqueDataSize << ", "
            << enclaveInfo.uniqueChunkNum << ", "
            << enclaveInfo.compressedSize << ", "
            << to_string(enclaveInfo.enclaveProcessTime) << endl;
        logFile_.flush();
    }

    // clean up
    for (auto it : thList) {
        delete it;
    }
    thList.clear();

    // clean up client variables
    delete curClient;
    free(recvBuf.sendBuffer);
    tmpLock->unlock();

    tool::Logging(myName_.c_str(), "total running time of client %u: %lf\n",
        clientID, totalTime);

    return ;
}

/**
 * @brief check the file status
 * 
 * @param fullRecipePath the full recipe path
 * @param optType the operation type
 * @return true success
 * @return false fail
 */
bool ServerOptThread::CheckFileStatus(string& fullRecipePath, int optType) {
    if (tool::FileExist(fullRecipePath)) {
        // the file exists
        switch (optType) {
            case UPLOAD_OPT: {
                tool::Logging(myName_.c_str(), "%s exists, overwrite it.\n",
                    fullRecipePath.c_str());
                break;
            }
            case DOWNLOAD_OPT: {
                tool::Logging(myName_.c_str(), "%s exists, access it.\n",
                    fullRecipePath.c_str());
                break;
            }
        }
    } else {
        switch (optType) {
            case UPLOAD_OPT: {
                tool::Logging(myName_.c_str(), "%s not exists, create it.\n",
                    fullRecipePath.c_str());
                break;
            }
            case DOWNLOAD_OPT: {
                tool::Logging(myName_.c_str(), "%s not exists, restore reject.\n",
                    fullRecipePath.c_str());
                return false;
            }
        }
    }
    return true;
}