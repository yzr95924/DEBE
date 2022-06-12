/**
 * @file sslConnect.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of SSLConnection
 * @version 0.1
 * @date 2021-01-20
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/sslConnection.h"

/**
 * @brief Construct a new SSLConnection object
 * 
 * @param ip the ip address
 * @param port the port number
 * @param type the type (client/server)
 */
SSLConnection::SSLConnection(string ip, int port, int type) {
    
    serverIP_ = ip;
    port_ = port;
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    
    // init the SSL lib
    SSL_library_init();
    SSL_load_error_strings();
    memset(&socketAddr_, 0, sizeof(socketAddr_));
    
    socketAddr_.sin_port = htons(port_);
    socketAddr_.sin_family = AF_INET;

    // load the cert and key
    string keyFileStr;
    string crtFileStr;
    string caFileStr;

    caFileStr.assign(CA_CERT);
    int enable = 1;
    switch (type) {
        case IN_SERVERSIDE:
            sslCtx_ = SSL_CTX_new(TLS_server_method());
            // need to reconsider this option when using epoll
            SSL_CTX_set_mode(sslCtx_, SSL_MODE_AUTO_RETRY); // handle for multiple time hand shakes
            keyFileStr.assign(SERVER_KEY);
            crtFileStr.assign(SERVER_CERT);
            socketAddr_.sin_addr.s_addr = htons(INADDR_ANY);
            if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
                tool::Logging(myName_.c_str(), "cannot set the port reusable.\n");
                exit(EXIT_FAILURE);
            }
            if (bind(listenFd_, (struct sockaddr*)&socketAddr_, sizeof(socketAddr_)) == -1) {
                tool::Logging(myName_.c_str(), "cannot not bind to socketFd\n"
                                "\tMay cause by shutdown server before client\n"
                                "\tWait for 1 min and try again\n");
                tool::Logging(myName_.c_str(), "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (listen(listenFd_, 10) == -1) {
                tool::Logging(myName_.c_str(), "cannot listen this socket.\n");
                tool::Logging(myName_.c_str(), "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            break;
        case IN_CLIENTSIDE:
            sslCtx_ = SSL_CTX_new(TLS_client_method());
            keyFileStr.assign(CLIENT_KEY);
            crtFileStr.assign(CLIENT_CERT);
            socketAddr_.sin_addr.s_addr = inet_addr(serverIP_.c_str());
            break;
        default:
            tool::Logging(myName_.c_str(), "error connection type.\n");
            exit(EXIT_FAILURE);
    }

    SSL_CTX_set_verify(sslCtx_, SSL_VERIFY_PEER, NULL);
    if (!SSL_CTX_load_verify_locations(sslCtx_, caFileStr.c_str(), NULL)) {
        tool::Logging(myName_.c_str(), "load ca crt error\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (!SSL_CTX_use_certificate_file(sslCtx_, crtFileStr.c_str(), SSL_FILETYPE_PEM)) {
        tool::Logging(myName_.c_str(), "load cert error.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (!SSL_CTX_use_PrivateKey_file(sslCtx_, keyFileStr.c_str(), SSL_FILETYPE_PEM)) {
        tool::Logging(myName_.c_str(), "load private key error.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (!SSL_CTX_check_private_key(sslCtx_)) {
        tool::Logging(myName_.c_str(), "check private key error.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    switch (type) {
        case IN_SERVERSIDE:
            tool::Logging(myName_.c_str(), "init the connection to port %d\n", port_);
            break;
        case IN_CLIENTSIDE:
            tool::Logging(myName_.c_str(), "init the connection to <%s:%d>\n", serverIP_.c_str(), port_);
            break; 
        default:
            tool::Logging(myName_.c_str(), "error connection type.\n");
            exit(EXIT_FAILURE);
    }
}


/**
 * @brief Destroy the SSLConnection object
 * 
 */
SSLConnection::~SSLConnection() {
    SSL_CTX_free(sslCtx_);
    close(listenFd_);
}

/**
 * @brief finalize the connection
 * 
 */
void SSLConnection::Finish(pair<int, SSL*> sslPair) {
    int ret = SSL_shutdown(sslPair.second);
    if (ret != 0) {
        tool::Logging(myName_.c_str(), "first shutdown the socket in client side error, "
                "ret: %d\n", ret);
        exit(EXIT_FAILURE);
    }

    // check the ssl shutdown flag state
    if ((SSL_get_shutdown(sslPair.second) & SSL_SENT_SHUTDOWN) != 1) {
        tool::Logging(myName_.c_str(), "set the sent shutdown flag error.\n");
    }

    // wait the close alert from another peer
    int tmp;
    int retStatus;
    retStatus = SSL_read(sslPair.second, (uint8_t*)&tmp, sizeof(tmp));
    if (SSL_get_error(sslPair.second, retStatus) != SSL_ERROR_ZERO_RETURN) {
        tool::Logging(myName_.c_str(), "receive shutdown flag error.\n");
    }
    tmp = SSL_shutdown(sslPair.second);
    if (tmp != 1) {
        tool::Logging(myName_.c_str(), "shutdown the ssl socket fail, ret: %d\n", tmp);
        exit(EXIT_FAILURE);
    }

    tool::Logging(myName_.c_str(), "shutdown the SSL connection successfully.\n");

    SSL_free(sslPair.second);
    close(sslPair.first);
    return ;
}

/**
 * @brief clear the corresponding accepted client socket and context
 * 
 * @param SSLPtr the pointer to the SSL* of accepted client
 */
void SSLConnection::ClearAcceptedClientSd(SSL* SSLPtr) {
    int sd = SSL_get_fd(SSLPtr);
    SSL_free(SSLPtr);
    close(sd);
    return ;
}


/**
 * @brief connect to ssl
 * 
 * @return pair<int, SSL*> 
 */
pair<int, SSL*> SSLConnection::ConnectSSL() {
    int socketFd;
    SSL* sslConnectionPtr;
    
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(socketFd, (struct sockaddr*)&socketAddr_, sizeof(socketAddr_)) < 0) {
        tool::Logging(myName_.c_str(), "cannot connect on the socket: %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    sslConnectionPtr = SSL_new(sslCtx_);
    if (!SSL_set_fd(sslConnectionPtr, socketFd)) {
        tool::Logging(myName_.c_str(), "cannot combine the fd and ssl.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    
    // start the SSL handshake 
    if (SSL_connect(sslConnectionPtr) != 1) {
        tool::Logging(myName_.c_str(), "ssl connect fails.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    } 

    return make_pair(socketFd, sslConnectionPtr);
}


/**
 * @brief listen to a port 
 * 
 * @return pair<int, SSL*> 
 */
pair<int, SSL*> SSLConnection::ListenSSL() {
    int socketFd;
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    socketFd = accept(listenFd_, (struct sockaddr*)&clientAddr, &clientAddrLen); 

    if (socketFd < 0) {
        tool::Logging(myName_.c_str(), "socket listen fails: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    SSL* sslConnectionPtr = SSL_new(sslCtx_);
    if (!SSL_set_fd(sslConnectionPtr, socketFd)) {
        tool::Logging(myName_.c_str(), "cannot combine the fd and ssl.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_accept(sslConnectionPtr) != 1) {
        tool::Logging(myName_.c_str(), "accept the connection fails.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return make_pair(socketFd, sslConnectionPtr);
}

/**
 * @brief send the data to the given connection
 * 
 * @param connection the pointer to the connection
 * @param data the pointer to the data buffer
 * @param dataSize the size of the input data
 * @return true success
 * @return false fail
 */
bool SSLConnection::SendData(SSL* connection, uint8_t* data, uint32_t dataSize) {
    int writeStatus;
    writeStatus = SSL_write(connection, (char*)&dataSize, sizeof(uint32_t));
    if (writeStatus <= 0) {
        tool::Logging(myName_.c_str(), "write the data fails. ret: %d\n", SSL_get_error(connection, writeStatus));
        ERR_print_errors_fp(stderr);
        return false;
    }

    int sendedSize = 0;
    while (sendedSize < dataSize) {
        sendedSize += SSL_write(connection, data + sendedSize, dataSize - sendedSize);
    }

    return true;

}

/**
 * @brief receive the data from the given connection
 * 
 * @param connection the pointer to the connection
 * @param data the pointer to the data buffer
 * @param receiveDataSize the size of received data 
 * @return true success
 * @return false fail
 */
bool SSLConnection::ReceiveData(SSL* connection, uint8_t* data, uint32_t& receiveDataSize) {
    int receivedSize = 0;
    int len = 0;
    int readStatus;
    readStatus = SSL_read(connection, (char*)&len, sizeof(int));
    if (readStatus <= 0) {
        if (SSL_get_error(connection, readStatus) == SSL_ERROR_ZERO_RETURN) {
            tool::Logging(myName_.c_str(), "TLS/SSL peer has closed the connection.\n");
            // also close this connection 
            SSL_shutdown(connection);
        }
        ERR_print_errors_fp(stderr);
        return false;
    }

    while (receivedSize < len) {
        receivedSize += SSL_read(connection, data + receivedSize, len - receivedSize);
    }
    receiveDataSize = len;
    return true;
}


/**
 * @brief Get the Client Ip object
 * 
 * @param ip the ip of the client 
 * @param clientSSL the SSL connection of the client
 */
void SSLConnection::GetClientIp(string& ip, SSL* clientSSL) {
    int clientFd = SSL_get_fd(clientSSL);
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    getpeername(clientFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
    ip.resize(INET_ADDRSTRLEN, 0);
    inet_ntop(AF_INET, &(clientAddr.sin_addr), &ip[0], INET_ADDRSTRLEN);
    return ;
}