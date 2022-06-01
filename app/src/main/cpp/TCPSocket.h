//
// Created by Administrator on 2022/4/11.
//

#ifndef NDKDEMO_TCPSOCKET_H
#define NDKDEMO_TCPSOCKET_H

#include <sys/types.h>

class TCPSocket {
public:
    TCPSocket();
    ~TCPSocket();

    int open(const char *ip, int port);

    ssize_t recv(void *buf, size_t len);

    ssize_t send(const void *buf, size_t len);

    int close();

private:
    int m_fd;
};


#endif //NDKDEMO_TCPSOCKET_H
