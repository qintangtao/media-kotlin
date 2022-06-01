//
// Created by Administrator on 2022/4/12.
//

#include "YuvData.h"
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <string>

#include "TCPSocket.h"
#include "AMediaDecoder.h"

#include <android/log.h>
#define TAG "YuvData"

#if 1
#define LOGV(...)
#define LOGE(...)
#else
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#endif


#define ENABLED_SAVE_H264           0
#define ENABLED_SAVE_SPLIT_H264     0

void *run_h264(void *arg)
{
    LOGV("enter h264 thread: ");
    ((YuvData*)arg)->run_h264_data();
    LOGV("leve h264 thread: ");
    return NULL;
}

void *run_yuv(void *arg)
{
    LOGV("enter yuv thread: ");
    ((YuvData*)arg)->run_yuv_data();
    LOGV("level yuv thread: ");
    return NULL;
}


YuvData::YuvData()
{
    m_h264_thread = 0;
    m_yuv_thread = 0;

    m_socket = 0;
    m_fp = 0;

    m_decoder = 0;

    m_yuv_queue = 0;

    m_max_h264_queue = 16;
    m_max_yuv_queue = 9;

    m_h264_buf = 0;
    m_yuv_buf = 0;
}
YuvData::~YuvData()
{
    close();
}

int YuvData::init(const char *ip, int port, int width, int height, int fmt, int test, threadsafe_queue<media::Frame *> *yuv_queue)
{
    m_yuv_queue = yuv_queue;

    if (1 == test)
        init_file();
    else
        init_socket(ip, port);
    init_decoder(width, height, fmt);
    init_data();
    start();
    return 0;
}

void YuvData::close()
{
    close_socket(false);
    stop();
    close_file();
    close_decoder();
    close_socket(true);
    close_data();
    clear_data();
}

int YuvData::start()
{
    int err;
    err = pthread_create(&m_h264_thread, NULL, run_h264, this);
    if (err != 0) {
        LOGE("can't create h264 thread: %s\n", strerror(err));
        m_h264_thread = 0;
        return err;
    }

    err = pthread_create(&m_yuv_thread, NULL, run_yuv, this);
    if (err != 0) {
        LOGE("can't create yuv thread: %s\n", strerror(err));
        pthread_join(m_h264_thread, 0);
        m_h264_thread = 0;
        m_yuv_thread = 0;
        return err;
    }

    LOGV("start h264 thread: %d\n", m_h264_thread);
    LOGV("start yuv thread: %d\n", m_yuv_thread);

    return err;
}

int YuvData::stop()
{
    int err = 0;

    m_yuv_exit = true;
    m_h264_exit = true;

    if (0 != m_yuv_thread) {
        err = pthread_join(m_yuv_thread, 0);
        LOGV("stop yuv thread: %d\n", m_yuv_thread);
        m_yuv_thread = 0;
    }

    if (0 != m_h264_thread) {
        err = pthread_join(m_h264_thread, 0);
        LOGV("stop h264 thread: %d\n", m_h264_thread);
        m_h264_thread = 0;
    }

    return err;
}

void YuvData::run_h264_data()
{
    ssize_t len;
    ssize_t offset;
    uint8_t buf[8*1024];
    uint8_t *start;
    uint8_t *end;

#if ENABLED_SAVE_H264
    std::string name = "/data/user/0/com.tang.ndkdemo/files/";
    name += std::to_string(1);
    name += ".h264";

    LOGV("file name is %s", name.c_str());
    FILE *fp = fopen(name.c_str(), "w");
    if (fp)
        LOGV("create file succeed %s", name.c_str());
    else
        LOGV("create file failed %s", strerror(errno));
#endif

    offset = 0;
    m_h264_exit = false;

    while (!m_h264_exit) {
        if (m_socket)
            len = m_socket->recv(buf, 8*1024);
        else if (m_fp)
            len = fread(buf, 1, 4*1024, m_fp);
        else
            break;
        if (len > 0) {

#if ENABLED_SAVE_H264
            if (fp)
                fwrite(buf, 1, len, fp);
#endif

            //LOGV("%d, %x, %x, %x, %x", len, buf[0], buf[1], buf[2], buf[3]);

#if 0
            push_h264_queue(buf, len);
#else
            if (offset + len > 8*1024*1024)
                offset = 0;

            memcpy(m_h264_buf + offset, buf, len);

            offset += len;

            start = split_h264_data(m_h264_buf, offset);
            if (start > m_h264_buf) {
                //剩余数据复制到起始位置
                end = m_h264_buf + offset;
                len = end - start;
                offset = len;
                memcpy(m_yuv_buf, start, len);
                memcpy(m_h264_buf, m_yuv_buf, len);

                LOGV("h264 queue size %d", m_h264_queue.size());
#if 1
                while (!m_h264_exit && m_h264_queue.size() >= m_max_h264_queue)
                    usleep(1000);

#else
                if (m_fp)
                    usleep(30000);
#endif
            }

#endif

        } else {
            if (m_socket) {
                if (errno != EINTR) {
                    LOGE("socket closed errno=%d", errno);
                    break;
                }
            } else if (m_fp)
              fseek(m_fp, 0, SEEK_SET);
        }
    }

#if ENABLED_SAVE_H264
    if (fp)
        fclose(fp);
#endif
}

uint8_t *YuvData::split_h264_data(uint8_t *data, size_t size)
{
    uint8_t *r, *r1, *r2=data, *end = data + size;

    r = media::find_startcode(data, end);
    while (0 != r && r < end)
    {
        r1 = media::find_startcode(r+2, end);
        if (0 == r1)
            break;

        r2 = r1;

        push_h264_queue(r, r1 - r);

        r = r1;
    }

    return r2;
}

void YuvData::push_h264_queue(uint8_t *data, size_t size)
{
    media::Packet *p = media::packet_alloc();
    media::packet_fill(p, data, size);

    m_h264_queue.push(p);

    LOGV("pushd h264 data len %d", size);
}


void YuvData::run_yuv_data()
{
    media::Packet *packet;

#if ENABLED_SAVE_SPLIT_H264
    std::string h264_name = "/data/user/0/com.tang.ndkdemo/files/";
    h264_name += std::to_string(2);
    h264_name += ".h264";

    LOGV("file name is %s", h264_name.c_str());
    FILE *h264_fp = fopen(h264_name.c_str(), "w");
    if (h264_fp)
        LOGV("create file succeed %s", h264_name.c_str());
    else
        LOGV("create file failed %s", strerror(errno));
#endif


    m_yuv_exit = false;
    while (!m_yuv_exit) {

        //LOGV("h264 queue count %d", m_h264_queue.size());
        while (!m_yuv_exit && m_h264_queue.try_pop(packet)) {
            //LOGV("decoder data len %d", len);

#if ENABLED_SAVE_SPLIT_H264
            if (h264_fp)
                fwrite(p_data, 1, len, h264_fp);
#endif

            if (m_decoder && m_yuv_queue)
                m_decoder->decoder(packet->data, packet->size, *m_yuv_queue);

            media::packet_free(packet);

            LOGV("yuv queue size %d", m_yuv_queue->size());
            while (!m_yuv_exit && m_yuv_queue->size() >= m_max_yuv_queue)
                usleep(1000);
        }

        usleep(1000);
    }

#if ENABLED_SAVE_SPLIT_H264
    if (h264_fp)
        fclose(h264_fp);
#endif
}

void YuvData::init_data()
{
    if (0 == m_h264_buf)
        m_h264_buf = (uint8_t *)malloc(8*1024*1024);

    if (0 == m_yuv_buf)
        m_yuv_buf = (uint8_t *)malloc(8*1024*1024);
}

void YuvData::close_data()
{
    if (0 != m_h264_buf)
    {
        free(m_h264_buf);
        m_h264_buf = 0;
    }

    if (0 != m_yuv_buf)
    {
        free(m_yuv_buf);
        m_yuv_buf = 0;
    }
}

int YuvData::init_socket(const char *ip, int port)
{
    m_socket = new TCPSocket();
    return m_socket->open(ip, port);
}

void YuvData::close_socket(bool del)
{
    if (0 != m_socket) {
        m_socket->close();
        if (del) {
            delete m_socket;
            m_socket = 0;
        }
    }
}

int YuvData::init_file()
{
    std::string name = "/data/local/tmp/1080P.h264";
    LOGV("init file name %s", name.c_str());
    m_fp = fopen(name.c_str(), "r");
    if (!m_fp) {
        LOGE("open file failed %s", strerror(errno));
        return -1;
    }

    LOGV("open file succeed %s", name.c_str());
    return 0;
}

void YuvData::close_file()
{
    if (0 != m_fp)
    {
        fclose(m_fp);
        m_fp = 0;
    }
}

int YuvData::init_decoder(int width, int height, int fmt)
{
    m_decoder = new AMediaDecoder("video/avc");
    return m_decoder->open(width, height, fmt);
}

void YuvData::close_decoder()
{
    if (0 != m_decoder) {
        m_decoder->close();
        delete m_decoder;
        m_decoder = 0;
    }
}

void YuvData::clear_data()
{
    m_h264_queue.clear();
}