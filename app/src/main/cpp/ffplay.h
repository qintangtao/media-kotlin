//
// Created by Administrator on 2022/5/5.
//

#ifndef NDKDEMO_FFPLAY_H
#define NDKDEMO_FFPLAY_H

typedef struct VideoState VideoState;
typedef struct AVInputFormat AVInputFormat;

VideoState *stream_open(const char *filename, AVInputFormat *iformat, void *surface);

void stream_close(VideoState *is);

void toggle_pause(VideoState *is);

void toggle_mute(VideoState *is);

void seek_chapter(VideoState *is, int incr);

void step_to_next_frame(VideoState *is);

void update_volume(VideoState *is, int sign, double step);

const char *stream_filename(VideoState *is);

void *stream_surface(VideoState *is);

#endif //NDKDEMO_FFPLAY_H
