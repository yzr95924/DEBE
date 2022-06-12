/**
 * @file sslConnection.hj
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of ssl connection
 * @version 0.1
 * @date 2021-01-20
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef SSL_CONNECTION_H
#define SSL_CONNECTION_H

#include "configure.h"

#include <netinet/in.h> // for sockaddr_in 
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>


class SSLConnection {
    private:
        string myName_ = "SSLConnection";
        // the socket address
        struct sockaddr_in socketAddr_;

        // server ip
        string serverIP_;

        // the port 
        int port_;

        // ssl context pointer
        SSL_CTX* sslCtx_ = NULL;

        // the listen file descriptor
        int listenFd_;

    public:
        /**
         * @brief Construct a new SSLConnection object
         * 
         * @param ip the ip address
         * @param port the port number
         * @param type the type (client/server)
         */
        SSLConnection(string ip, int port, int type);

        /**
         * @brief Destroy the SSLConnection object
         * 
         */
        ~SSLConnection();

        /**
         * @brief finalize the connection
         * 
         * @param sslPair the pair of the server socket and ssl context
         */
        void Finish(pair<int, SSL*> sslPair);

        /**
         * @brief clear the corresponding accepted client socket and context
         * 
         * @param SSLPtr the pointer to the SSL* of accepted client
         */
        void ClearAcceptedClientSd(SSL* SSLPtr);

        /**
         * @brief connect to ssl
         * 
         * @return pair<int, SSL*> 
         */
        pair<int, SSL*> ConnectSSL();

        /**
         * @brief listen to a port 
         * 
         * @return pair<int, SSL*> 
         */
        pair<int, SSL*> ListenSSL();

        /**
         * @brief send the data to the given connection
         * 
         * @param connection the pointer to the connection
         * @param data the pointer to the data buffer
         * @param dataSize the size of the input data
         * @return true success
         * @return false fail
         */
        bool SendData(SSL* connection, uint8_t* data, uint32_t dataSize);

        /**
         * @brief receive the data from the given connection
         * 
         * @param connection the pointer to the connection
         * @param data the pointer to the data buffer
         * @param receiveDataSize the size of received data 
         * @return true success
         * @return false fail
         */
        bool ReceiveData(SSL* connection, uint8_t* data, uint32_t& receiveDataSize);

        /**
         * @brief Get the Listen Fd object
         * 
         * @return int the listenFd
         */
        inline int GetListenFd() {
            return this->listenFd_;
        }

        /**
         * @brief Get the Client Ip object
         * 
         * @param ip the ip of the client 
         * @param clientSSL the SSL connection of the client
         */
        void GetClientIp(string& ip, SSL* clientSSL);

};

#endif
