//
// Created by Administrator on 2022/4/14.
//

#ifndef NDKDEMO_RGBDATA_H
#define NDKDEMO_RGBDATA_H
#include "threadsafe_queue.h"
#include "common.h"

class YuvData;

class RGBData {
public:
    RGBData();
    ~RGBData();

public:
    int init(const char *ip, int port, int test);
    void close();

    threadsafe_queue<media::Rgb *> &get_rgb_queue()
    { return m_rgb_queue; }

    void run_rgb_data();

private:
    void push_rgb_queue(uint8_t *data, int width, int height, int pixelCount);

private:
    void init_data();
    void close_data();

    int init_yuv(const char *ip, int port, int test);
    void close_yuv();

    int start();
    int stop();

private:
    pthread_t m_rgb_thread;
    bool m_rgb_exit;

    uint8_t *m_rgb_buf;

    YuvData *m_yuvData;

    int m_width;
    int m_height;

    threadsafe_queue<media::Frame *> m_yuv_queue;
    threadsafe_queue<media::Rgb *> m_rgb_queue;

    uint8_t m_max_queue;
};


#endif //NDKDEMO_RGBDATA_H
