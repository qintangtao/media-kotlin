//
// Created by Administrator on 2022/4/11.
//

#ifndef NDKDEMO_AMEDIADECODER_H
#define NDKDEMO_AMEDIADECODER_H
#include <queue>
#include <string>
#include "threadsafe_queue.h"
#include "common.h"

typedef struct AMediaCodec AMediaCodec;

class AMediaDecoder {
public:
    AMediaDecoder(const char *mime_type);
    ~AMediaDecoder();

    int open(int width, int height, int fmt);

    int close();

    int decoder(const uint8_t *data, int size, threadsafe_queue<media::Frame *> &queue);

    inline int fmt()
    { return m_fmt; }

    inline int widget()
    { return m_width; }

    inline int height()
    { return m_height; }

private:
    AMediaCodec *m_decoder;

    std::string m_mime_type;
    int m_width;
    int m_height;
    int m_fmt;

    int m_frame_count;
    int64_t m_begin_time;
};


#endif //NDKDEMO_AMEDIADECODER_H
