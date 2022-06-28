//
// Created by Administrator on 2022/4/18.
//

#ifndef NDKDEMO_COMMON_H
#define NDKDEMO_COMMON_H

#include <stdint.h>

namespace media {

#pragma pack (1)

typedef struct Packet {
    uint32_t size;
    uint8_t *data;
} Packet;

typedef struct Frame {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t fmt;
    uint8_t *data;
} Frame;

typedef struct Rgb {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t fmt;
    uint8_t *data;
} Rgb;

#pragma pack ()

Rgb *rgb_alloc();

void rgb_free(Rgb *rgb);

void rgb_fill(Rgb *rgb, uint8_t *data, uint32_t size, uint32_t width, uint32_t height, uint32_t fmt);


Frame *frame_alloc();

void frame_free(Frame *f);

void frame_fill(Frame *f, uint8_t *data, uint32_t size, uint32_t width, uint32_t height, uint32_t fmt);


Packet *packet_alloc();

void packet_free(Packet *p);

void packet_fill(Packet *p, uint8_t *data, uint32_t size);

uint8_t *find_startcode(uint8_t *p, uint8_t *end);


} //end namespace media

#endif //NDKDEMO_COMMON_H
