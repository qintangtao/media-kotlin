//
// Created by Administrator on 2022/4/12.
//

#ifndef NDKDEMO_YUVDATA_H
#define NDKDEMO_YUVDATA_H

#include <list>
#include "threadsafe_queue.h"
#include "common.h"

typedef long pthread_t;

class TCPSocket;
class AMediaDecoder;

class YuvData {

public:
    YuvData();
    ~YuvData();

    int init(const char *ip,
            int port,
            int width,
            int height,
            int fmt,
            int test,
            threadsafe_queue<media::Frame *> *yuv_queue);

    void close();

    void run_h264_data();

    void run_yuv_data();

private:

    uint8_t * split_h264_data(uint8_t *data, size_t size);

    void push_h264_queue(uint8_t *data, size_t size);

private:
    int start();
    int stop();

    void init_data();
    void close_data();

    int init_socket(const char *ip, int port);
    void close_socket(bool del);

    int init_file();
    void close_file();

    int init_decoder(int width, int height, int fmt);
    void close_decoder();

    void clear_data();

private:
    pthread_t m_h264_thread;
    pthread_t m_yuv_thread;

    bool m_h264_exit;
    bool m_yuv_exit;

    uint8_t *m_h264_buf;
    uint8_t *m_yuv_buf;

    threadsafe_queue<media::Packet *> m_h264_queue;
    threadsafe_queue<media::Frame *> *m_yuv_queue;

    uint8_t m_max_h264_queue;
    uint8_t m_max_yuv_queue;

    TCPSocket *m_socket;
    FILE *m_fp;

    AMediaDecoder *m_decoder;
};


#endif //NDKDEMO_YUVDATA_H
