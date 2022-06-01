//
// Created by Administrator on 2022/4/11.
//

#include "AMediaDecoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <media/NdkMediaCodec.h>
#include "time_util.h"

#include <android/log.h>
#define TAG "AMediaDecoder"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)


#define ENABLED_CALC_PTS 0

AMediaDecoder::AMediaDecoder(const char *mime_type)
{
    m_decoder = NULL;

    m_mime_type = mime_type;
    m_width = 0;
    m_height = 0;
    m_fmt = 0;

    m_frame_count = 0;
    m_begin_time = 0;
}

AMediaDecoder::~AMediaDecoder()
{
    close();
}

int AMediaDecoder::open(int width, int height, int fmt)
{
    media_status_t status;
    AMediaFormat *format;

    /*
        ● “video/x-vnd.on2.vp8” - VP8 video (i.e. video in .webm)
        ● “video/x-vnd.on2.vp9” - VP9 video (i.e. video in .webm)
        ● “video/avc” - H.264/AVC video
        ● “video/mp4v-es” - MPEG4 video
        ● “video/3gpp” - H.263 video
        ● “audio/3gpp” - AMR narrowband audio
        ● “audio/amr-wb” - AMR wideband audio
        ● “audio/mpeg” - MPEG1/2 audio layer III
        ● “audio/mp4a-latm” - AAC audio (note, this is raw AAC packets, not packaged in LATM!)
        ● “audio/vorbis” - vorbis audio
        ● “audio/g711-alaw” - G.711 alaw audio
        ● “audio/g711-mlaw” - G.711 ulaw audio
     * */
    m_decoder = AMediaCodec_createDecoderByType(m_mime_type.c_str());
    //注意这里，如果系统不存在对应的mine，mMediaCodec也会返回对象，
    //不过之后涉及到mMediaCodec的任何函数都会直接导致程序崩掉。
    if (NULL == m_decoder) {
        LOGE("AMediaCodec_createDecoderByType error");
        return -1;
    }

    format = AMediaFormat_new();
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, m_mime_type.c_str());
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, height);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, fmt);

    status = AMediaCodec_configure(m_decoder, format, NULL, NULL, 0);
    if (status != AMEDIA_OK) {
        LOGE("AMediaCodec_configure error: %d", status);
        AMediaFormat_delete(format);
        AMediaCodec_delete(m_decoder);
        m_decoder = NULL;
        return -1;
    }

    AMediaFormat_delete(format);

    m_width = width;
    m_height = height;
    m_fmt = fmt;

    status = AMediaCodec_start(m_decoder);
    if (status != AMEDIA_OK) {
        LOGE("AMediaCodec_start error: %d", status);
        AMediaCodec_delete(m_decoder);
        m_decoder = NULL;
        return -1;
    }

    format = AMediaCodec_getOutputFormat(m_decoder);
    if (format) {
        AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &m_width);
        AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &m_height);
        AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &m_fmt);
        AMediaFormat_delete(format);
    }

    LOGV("init decoder successfully: width = %d, height = %d, pix_fmt = %d", m_width, m_height, m_fmt);

    return 0;
}


int AMediaDecoder::close()
{
    if (NULL == m_decoder) {
        LOGE("AMediaCodec id null");
        return -1;
    }

    AMediaCodec_stop(m_decoder);
    AMediaCodec_delete(m_decoder);
    m_decoder = NULL;

    LOGV("close decoder successfully: width = %d, height = %d, pix_fmt = %d", m_width, m_height, m_fmt);

    return 0;
}

int AMediaDecoder::decoder(const uint8_t *data, int size, threadsafe_queue<media::Frame *> &queue)
{
    ssize_t idx;
    size_t outsize;
    uint8_t *buf;
    uint8_t *yuv_buf;
    AMediaCodecBufferInfo info;

    if (NULL == m_decoder) {
        LOGE("AMediaCodec id null");
        return -1;
    }

    idx = AMediaCodec_dequeueInputBuffer(m_decoder, -1);
    if (idx >= 0) {
        buf = AMediaCodec_getInputBuffer(m_decoder, idx, &outsize);
        if (buf && size <= outsize) {
            memcpy(buf, data, size);
            AMediaCodec_queueInputBuffer(m_decoder, idx, 0, size, gettime_relative(), 0);
        }
    }

    while ( (idx = AMediaCodec_dequeueOutputBuffer(m_decoder, &info, 0)) > 0) {

        //LOGV("decoder: offset=%d, size=%d, presentationTimeUs=%lld flags=%d",
         //    info.offset, info.size, info.presentationTimeUs, info.flags);

        buf = AMediaCodec_getOutputBuffer(m_decoder, idx, &outsize);
        if (buf && outsize > 0) {
            media::Frame *frame = media::frame_alloc();
            media::frame_fill(frame, buf, outsize, m_width, m_height, m_fmt);
            queue.push(frame);
        }

        AMediaCodec_releaseOutputBuffer(m_decoder, idx, false);

#if ENABLED_CALC_PTS
        if (0 == m_frame_count) {
            //开始计时
            m_begin_time = getTimeUsec();
        }

        m_frame_count++;

        uint64_t end_time = getTimeUsec();
        uint64_t diff = end_time - m_begin_time;
        if ( diff > 1000000*30 )
        {
            LOGV("decoder time: start_time=%lld, end_time=%lld, diff=%lld, frame_count=%d",
                    m_begin_time, end_time, diff, m_frame_count);
            m_frame_count = 0;
        }
#endif
    }

    return 0;
}