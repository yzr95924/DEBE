/**
 * @file clientVar.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of client var
 * @version 0.1
 * @date 2021-04-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/clientVar.h"

/**
 * @brief Construct a new Client Var object
 * 
 * @param clientID the client ID
 * @param clientSSL the client SSL
 * @param optType the operation type (upload / download)
 * @param recipePath the file recipe path
 */
ClientVar::ClientVar(uint32_t clientID, SSL* clientSSL, 
    int optType, string& recipePath) {
    // basic info
    _clientID = clientID;
    _clientSSL = clientSSL;
    optType_ = optType;
    recipePath_ = recipePath;
    myName_ = myName_ + "-" + to_string(_clientID);

    // config
    sendChunkBatchSize_ = config.GetSendChunkBatchSize();
    sendRecipeBatchSize_ = config.GetSendRecipeBatchSize();

    switch (optType_) {
        case UPLOAD_OPT: {
            this->InitUploadBuffer();
            break;
        }
        case DOWNLOAD_OPT: {
            this->InitRestoreBuffer();
            break;
        }
        default: {
            tool::Logging(myName_.c_str(), "wrong client opt type.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Destroy the Client Var object
 * 
 */
ClientVar::~ClientVar() {
    switch (optType_) {
        case UPLOAD_OPT: {
            this->DestoryUploadBuffer();
            break;
        }
        case DOWNLOAD_OPT: {
            this->DestoryRestoreBuffer();
            break;
        }
    }
}

/**
 * @brief init the upload buffer
 * 
 */
void ClientVar::InitUploadBuffer() {
    // assign a random id to the container
    tool::CreateUUID(_curContainer.containerID, CONTAINER_ID_LENGTH);
    _curContainer.currentSize = 0;

    // init the recv buffer
    _recvChunkBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendChunkBatchSize_ * (sizeof(uint32_t) + MAX_CHUNK_SIZE));
    _recvChunkBuf.header = (NetworkHead_t*) _recvChunkBuf.sendBuffer;
    _recvChunkBuf.header->clientID = _clientID;
    _recvChunkBuf.header->currentItemNum = 0;
    _recvChunkBuf.header->dataSize = 0;
    _recvChunkBuf.dataBuffer = _recvChunkBuf.sendBuffer + sizeof(NetworkHead_t);

    // init the recipe buffer
    _inRecipe.entryList = (uint8_t*) malloc(sendRecipeBatchSize_ *
        sizeof(RecipeEntry_t));
    _inRecipe.recipeNum = 0;

    // prepare the input MQ
    _inputMQ = new MessageQueue<Container_t>(CONTAINER_QUEUE_SIZE);

    // prepare the crypto
    _mdCtx = EVP_MD_CTX_new();
    _cipherCtx = EVP_CIPHER_CTX_new();

    // init the file recipe
    _recipeWriteHandler.open(recipePath_, ios_base::trunc | ios_base::binary);
    if (!_recipeWriteHandler.is_open()) {
        tool::Logging(myName_.c_str(), "cannot init recipe file: %s\n",
            recipePath_.c_str());
        exit(EXIT_FAILURE);
    }
    FileRecipeHead_t virtualRecipeEnd;
    _recipeWriteHandler.write((char*)&virtualRecipeEnd, sizeof(FileRecipeHead_t));

    // for key recipe
    string keyRecipeName = recipePath_ + keyRecipeSuffix_;
    _keyRecipeWriteHandler.open(keyRecipeName, ios_base::trunc | ios_base::binary);
    if (!_keyRecipeWriteHandler.is_open()) {
        tool::Logging(myName_.c_str(), "cannot init key recipe file: %s\n",
            keyRecipeName.c_str());
        exit(EXIT_FAILURE);
    }

    return ;
}

/**
 * @brief destory the upload buffer
 * 
 */
void ClientVar::DestoryUploadBuffer() {
    if (_recipeWriteHandler.is_open()) {
        _recipeWriteHandler.close();
    }
    if (_keyRecipeWriteHandler.is_open()) {
        _keyRecipeWriteHandler.close();
    }
    free(_recvChunkBuf.sendBuffer);
    free(_inRecipe.entryList);
    delete _inputMQ;
    EVP_MD_CTX_free(_mdCtx);
    EVP_CIPHER_CTX_free(_cipherCtx);
    return ;
}

/**
 * @brief init the restore buffer
 * 
 */
void ClientVar::InitRestoreBuffer() {
    // init buffer
    _readRecipeBuf = (uint8_t*) malloc(sendRecipeBatchSize_ * sizeof(RecipeEntry_t));
    _reqContainer.idBuffer = (uint8_t*) malloc(CONTAINER_CAPPING_VALUE *
        CONTAINER_ID_LENGTH);
    _reqContainer.containerArray = (uint8_t**) malloc(CONTAINER_CAPPING_VALUE *
        sizeof(uint8_t*));
    _reqContainer.idNum = 0;
    for (size_t i = 0; i < CONTAINER_CAPPING_VALUE; i++) {
        _reqContainer.containerArray[i] = (uint8_t*) malloc(sizeof(uint8_t) *
            MAX_CONTAINER_SIZE);
    }

    // init the send chunk buffer
    _sendChunkBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendChunkBatchSize_ * (sizeof(uint32_t) + MAX_CHUNK_SIZE));
    _sendChunkBuf.header = (NetworkHead_t*) _sendChunkBuf.sendBuffer;
    _sendChunkBuf.header->clientID = _clientID;
    _sendChunkBuf.header->currentItemNum = 0;
    _sendChunkBuf.header->dataSize = 0;
    _sendChunkBuf.dataBuffer = _sendChunkBuf.sendBuffer + sizeof(NetworkHead_t);

    // init the container cache
    _containerCache = new ReadCache();

    // init the recipe handler
    _recipeReadHandler.open(recipePath_, ios_base::in | ios_base::binary);
    if (!_recipeReadHandler.is_open()) {
        tool::Logging(myName_.c_str(), "cannot init the file recipe: %s.\n",
            recipePath_.c_str());
        exit(EXIT_FAILURE);
    }
    _enclaveRecipeBuffer.reserve(sendRecipeBatchSize_);

    return ;
}

/**
 * @brief destory the restore buffer
 * 
 */
void ClientVar::DestoryRestoreBuffer() {
    free(_sendChunkBuf.sendBuffer);
    free(_readRecipeBuf);
    free(_reqContainer.idBuffer);
    for (size_t i = 0; i < CONTAINER_CAPPING_VALUE; i++) {
        free(_reqContainer.containerArray[i]);
    }
    free(_reqContainer.containerArray);
    delete _containerCache;
    return ;
}