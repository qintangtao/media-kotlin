//
// Created by Administrator on 2022/5/5.
//

#ifndef NDKDEMO_FFPLAY_H
#define NDKDEMO_FFPLAY_H
#include <stdint.h>

typedef struct VideoState VideoState;
typedef struct AVInputFormat AVInputFormat;

enum ffEvent{
    FF_EVENT_TOOGLE_PAUSE=0,      //暂停
    FF_EVENT_TOOGLE_MUTE,       //静音
    FF_EVENT_INC_VOLUME,        //静音加
    FF_EVENT_DEC_VOLUME,        //静音减
    FF_EVENT_NEXT_FRAME,         //下一帧
    FF_EVENT_FAST_BACK,         //后退
    FF_EVENT_FAST_FORWARD,      //快进

};

VideoState *stream_open(const char *filename, AVInputFormat *iformat, void *surface);

void stream_close(VideoState *is);

/* 通过事件控制 */
void send_event(VideoState *is, enum ffEvent e);

//seek by bytes 0=off 1=on -1=auto
void set_seek_by_bytes(VideoState *is, int seek_by_bytes);

void set_seek_interval(VideoState *is, int seek_interval);

bool is_paused(VideoState *is);

bool is_mute(VideoState *is);

const char *stream_filename(VideoState *is);

void *stream_surface(VideoState *is);

void set_volume(VideoState *is, int volume);

int64_t get_duration(VideoState *is);

int64_t get_current_duration(VideoState *is);

void seek_pos(VideoState *is, int64_t pos);

void set_rate(VideoState *is, int rate);

#endif //NDKDEMO_FFPLAY_H
