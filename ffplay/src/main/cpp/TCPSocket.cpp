//
// Created by Administrator on 2022/4/11.
//

#include "TCPSocket.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include <android/log.h>
#define TAG "TCPSocket"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

TCPSocket::TCPSocket()
{
    m_fd = -1;
}

TCPSocket::~TCPSocket()
{

}

int TCPSocket::open(const char *ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        LOGE("create socket failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in dst_addr;
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(port);
    dst_addr.sin_addr.s_addr = inet_addr(ip);
    bzero(&(dst_addr.sin_zero), 8);

    if (connect(sockfd, (struct sockaddr *)&dst_addr, sizeof(struct sockaddr)) < 0) {
        LOGE("connect failed: ip=%s, port=%d, error=%s", ip, port, strerror(errno));
        ::close(sockfd);
        return -1;
    }

    m_fd = sockfd;

    LOGV("connect succeed: ip=%s, port=%d, socket_fd=%d", ip, port, sockfd);

    return 0;
}


ssize_t TCPSocket::recv(void *buf, size_t len)
{
    return recvfrom(m_fd,buf, len, 0, 0, 0);
}

ssize_t TCPSocket::send(const void *buf, size_t len)
{
    return sendto(m_fd, buf, len, 0, 0, 0);
}

int TCPSocket::close()
{
    if (m_fd > 0) {
        ::shutdown(m_fd, SHUT_RDWR);
        ::close(m_fd);
        m_fd = -1;
        LOGV("close socket: socket_fd=%d", m_fd);
    }

    return 0;
}