//
// Created by Administrator on 2022/4/14.
//

#include "RGBData.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include "YuvData.h"
#include "yuv_util.h"

#include <android/log.h>
#define TAG "RGBData"
#if 1
#define LOGV(...)
#define LOGE(...)
#else
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#endif

#define ENABLED_SAVE_YUV           0
#define ENABLED_SAVE_RGB           0

#define ENABLED_RGB565                 1
#define ENABLED_RGBA8888               0

void *run_rgb(void *arg)
{
    LOGV("enter rgb thread: ");
        ((RGBData*)arg)->run_rgb_data();
    LOGV("level rgb thread: ");
    return NULL;
}

RGBData::RGBData()
{
    m_rgb_thread = 0;
    m_rgb_exit = false;
    m_rgb_buf = 0;
    m_yuvData = 0;
    m_width = 1920;
    m_height = 1080;
    m_max_queue = 6;
}

RGBData::~RGBData()
{

}

int RGBData::init(const char *ip, int port, int test)
{
    init_yuv(ip, port, test);
    init_data();
    return start();
}

void RGBData::close()
{
    stop();
    close_yuv();
    close_data();
    m_yuv_queue.clear();
    m_rgb_queue.clear();
}

void RGBData::run_rgb_data()
{
    media::Frame *frame;

#if ENABLED_SAVE_YUV
    std::string yuv_name = "/data/user/0/com.tang.ndkdemo/files/";
    yuv_name += std::to_string(0);
    yuv_name += ".yuv";

    LOGV("file name is %s", yuv_name.c_str());
    FILE *yuv_fp = fopen(yuv_name.c_str(), "w");
    if (yuv_fp)
        LOGV("create file succeed %s", yuv_name.c_str());
    else
        LOGV("create file failed %s", strerror(errno));
#endif

    while (!m_rgb_exit) {

        LOGV("yuv queue count %d", m_yuv_queue.size());
        while (!m_rgb_exit && m_yuv_queue.try_pop(frame)) {

            //LOGV("yuv data len is %d", len);

#if ENABLED_SAVE_YUV
            if (yuv_fp)
                fwrite(p_data, 1, len, yuv_fp);
#endif

#if ENABLED_RGB565
            I420ToRGB565(frame->data, frame->width, frame->height, m_rgb_buf);

            push_rgb_queue(m_rgb_buf, frame->width, frame->height, 2);
#endif

#if ENABLED_RGBA8888
            I420ToRGBA(frame->data, frame->width, frame->height, m_rgb_buf);

            push_rgb_queue(m_rgb_buf, frame->width, frame->height, 4);
#endif

            media::frame_free(frame);

            while (!m_rgb_exit && m_rgb_queue.size() >= m_max_queue)
                usleep(1000);
        }

        usleep(1000);
    }

#if ENABLED_SAVE_YUV
    if (yuv_fp)
        fclose(yuv_fp);
#endif
}

void RGBData::push_rgb_queue(uint8_t *data, int width, int height, int pixelCount)
{
    ssize_t size = width * height * pixelCount;

    media::Rgb *rgb = media::rgb_alloc();
    media::rgb_fill(rgb, data, size, width, height, pixelCount);

    m_rgb_queue.push(rgb);

    LOGV("pushd rgb data len %d", size);

#if ENABLED_SAVE_RGB
    std::string rgb_name = "/data/user/0/com.tang.ndkdemo/files/";
    rgb_name += std::to_string(0);
    rgb_name += ".png";

    int r = saveBMP(rgb_name.c_str(), data, width, height, pixelCount);

#if 0
    FILE *rgb_fp = fopen(rgb_name.c_str(), "w");
    if (rgb_fp) {
        LOGV("create file succeed %s", rgb_name.c_str());
        fwrite(m_rgb_buf, 1, size, rgb_fp);
        fclose(rgb_fp);
    }
    else
        LOGV("create file failed %s", strerror(errno));
#endif

#endif
}

void RGBData::init_data()
{
    if (0 == m_rgb_buf)
        m_rgb_buf = (uint8_t *)malloc(8*1024*1024);
}

void RGBData::close_data()
{
    if (0 != m_rgb_buf)
    {
        free(m_rgb_buf);
        m_rgb_buf = 0;
    }
}


int RGBData::init_yuv(const char *ip, int port, int test)
{
    //m_width = 1920;
    m_height = 1 == test ? 1080 : 1088;

    /*
public static final int COLOR_FormatYUV411Planar            = 17;
public static final int COLOR_FormatYUV411PackedPlanar      = 18;
public static final int COLOR_FormatYUV420Planar            = 19;   //I420
public static final int COLOR_FormatYUV420PackedPlanar      = 20;
public static final int COLOR_FormatYUV420SemiPlanar        = 21;
public static final int COLOR_FormatYUV422Planar            = 22;
public static final int COLOR_FormatYUV422PackedPlanar      = 23;
public static final int COLOR_FormatYUV422SemiPlanar        = 24;
*/

    m_yuvData = new YuvData();
    m_yuvData->init(ip,
                    port,
                    m_width,
                    m_height,
                    19,
                    test,
                    &m_yuv_queue);
    return 0;
}

void RGBData::close_yuv()
{
    if (0 != m_yuvData)
    {
        m_yuvData->close();
        delete m_yuvData;
        m_yuvData = 0;
    }
}

int RGBData::start()
{
    int err;
    err = pthread_create(&m_rgb_thread, NULL, run_rgb, this);
    if (err != 0) {
        LOGE("can't create h264 thread: %s\n", strerror(err));
        m_rgb_thread = 0;
        return err;
    }

    LOGV("start h264 thread: %d\n", m_rgb_thread);

    return err;
}

int RGBData::stop()
{
    int err = 0;

    if (0 != m_rgb_thread) {
        m_rgb_exit = true;
        err = pthread_join(m_rgb_thread, 0);
        LOGV("stop rgb thread: %d\n", m_rgb_thread);
        m_rgb_thread = 0;
    }

    return err;
}