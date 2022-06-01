//
// Created by Administrator on 2022/4/19.
//
#include "common.h"
#include <malloc.h>
#include <string.h>

namespace media {

Rgb *rgb_alloc()
{
    Rgb *rgb = (Rgb *)malloc(sizeof(Rgb));
    memset(rgb, 0, sizeof(Rgb));
    return rgb;
}

void rgb_free(Rgb *rgb)
{
    if (!rgb)
        return;

    if (rgb->data)
    {
        free(rgb->data);
        rgb->data = 0;
    }

    free(rgb);
    rgb = 0;
}

void rgb_fill(Rgb *rgb, uint8_t *data, uint32_t size, uint32_t width, uint32_t height, uint32_t fmt)
{
    if (!rgb)
        return;

    if (size > 0)
    {
        rgb->size = size;
        rgb->data = (uint8_t *) malloc(rgb->size);
        memcpy(rgb->data, data, rgb->size);
    }

    rgb->width = width;
    rgb->height = height;
    rgb->fmt = fmt;
}



Frame *frame_alloc()
{
    Frame *f = (Frame *)malloc(sizeof(Frame));
    memset(f, 0, sizeof(Frame));
    return f;
}

void frame_free(Frame *f)
{
    if (!f)
        return;

    if (f->data)
    {
        free(f->data);
        f->data = 0;
    }

    free(f);
    f = 0;
}

void frame_fill(Frame *f, uint8_t *data, uint32_t size, uint32_t width, uint32_t height, uint32_t fmt)
{
    if (!f)
        return;

    if (size > 0)
    {
        f->size = size;
        f->data = (uint8_t *) malloc(f->size);
        memcpy(f->data, data, f->size);
    }

    f->width = width;
    f->height = height;
    f->fmt = fmt;
}

Packet *packet_alloc()
{
    Packet *p = (Packet *)malloc(sizeof(Packet));
    memset(p, 0, sizeof(Packet));
    return p;
}

void packet_free(Packet *p)
{
    if (!p)
        return;

    if (p->data)
    {
        free(p->data);
        p->data = 0;
    }

    free(p);
    p = 0;
}

void packet_fill(Packet *p, uint8_t *data, uint32_t size)
{
    if (!p)
        return;

    if (size <= 0)
        return;

    p->size = size;
    p->data = (uint8_t *) malloc(p->size);
    memcpy(p->data, data, p->size);
}


uint8_t *find_startcode_internal(uint8_t *p, uint8_t *end)
{
    for (end -= 4; p < end; p++)
    {
#if 1
        if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1)
        {
            return p;
        }
#else
        if (p[0] == 0 && p[1] == 0)
    {
        if (p[2] == 1)
            return p;
        if (p[2] == 0 && p[3] == 1)
            return p;
    }

    if (p + 4 == end)
    {
        if (p[1] == 0 && p[2] == 0 && p[3] == 1)
            return p+1;
    }
#endif
    }

    return 0;
}

uint8_t *find_startcode(uint8_t *p, uint8_t *end)
{
    return find_startcode_internal(p, end);
}


} //end namespace media