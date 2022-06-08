//
// Created by Administrator on 2022/5/5.
//

#include "ffplay.h"
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include "yuv_util.h"
extern "C"
{
#include "libavcodec/jni.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/mediacodec.h"
#include "libavutil/hwcontext_mediacodec.h"
#include "libavutil/time.h"
#include "libavutil/avutil.h"
#include "libavutil/channel_layout.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "opensl_io.h"
};


const char program_name[] = "ffplay";
const int program_birth_year = 2003;

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

#define CURSOR_HIDE_DELAY 1000000

#define USE_ONEPASS_SUBTITLE_RENDER 1

#ifndef SDL_MIX_MAXVOLUME
#define SDL_MIX_MAXVOLUME 128
#endif
//static unsigned sws_flags = SWS_BICUBIC;

typedef struct MyAVPacketList {
    AVPacket pkt;
    struct MyAVPacketList *next;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    int serial;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    VideoState *is;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

typedef struct Clock {
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame {
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;           /* presentation timestamp for the frame */
    double duration;      /* estimated duration of the frame */
    int64_t pos;          /* byte position of the frame in the input file */
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;
} Frame;

typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    PacketQueue *pktq;
    VideoState *is;
} FrameQueue;

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct Decoder {
    AVPacket pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    pthread_cond_t empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    pthread_t decoder_tid;
    VideoState *is;
} Decoder;


typedef struct MyEvent {
    enum ffEvent code;
    uint8_t *data;
    int   size;
}MyEvent;

typedef struct MyEventList {
    MyEvent evt;
    struct MyEventList *next;
} MyEventList;

typedef struct EventQueue {
    MyEventList *first_event, *last_event;
    int nb_events;
    int size;
    int abort_request;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    VideoState *is;
} EventQueue;

typedef struct Looper {
    EventQueue *queue;
    pthread_t looper_tid;
} Looper;


typedef struct VideoState {

    Looper looper;
    EventQueue eventq;

    OPENSL_STREAM* audios;

    pthread_t read_tid;
    AVInputFormat *iformat;
    int abort_request;
    int force_refresh;
    int paused;
    int last_paused;
    int queue_attachments_req;
    int seek_req;
    int seek_flags;
    int64_t seek_pos;
    int64_t seek_rel;
    int read_pause_return;
    AVFormatContext *ic;
    int realtime;

    Clock audclk;
    Clock vidclk;
    Clock extclk;

    FrameQueue pictq;
    FrameQueue subpq;
    FrameQueue sampq;

    Decoder auddec;
    Decoder viddec;
    Decoder subdec;

    int audio_stream;

    int av_sync_type;

    double audio_clock;
    int audio_clock_serial;
    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    AVStream *audio_st;
    PacketQueue audioq;
    int audio_hw_buf_size;
    uint8_t *audio_buf;
    uint8_t *audio_buf1;
    unsigned int audio_buf_size; /* in bytes */
    unsigned int audio_buf1_size;
    int audio_buf_index; /* in bytes */
    int audio_write_buf_size;
    int audio_volume;
    int muted;
    struct AudioParams audio_src;
#if CONFIG_AVFILTER
    struct AudioParams audio_filter_src;
#endif
    struct AudioParams audio_tgt;
    struct SwrContext *swr_ctx;
    int frame_drops_early;
    int frame_drops_late;

    enum ShowMode {
        SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
    } show_mode;
    int16_t sample_array[SAMPLE_ARRAY_SIZE];
    int sample_array_index;
    int last_i_start;
    //RDFTContext *rdft;
    int rdft_bits;
    //FFTSample *rdft_data;
    int xpos;
    double last_vis_time;
    //SDL_Texture *vis_texture;
    //SDL_Texture *sub_texture;
    //SDL_Texture *vid_texture;

    int subtitle_stream;
    AVStream *subtitle_st;
    PacketQueue subtitleq;

    double frame_timer;
    double frame_last_returned_time;
    double frame_last_filter_delay;
    int video_stream;
    AVStream *video_st;
    PacketQueue videoq;
    double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    struct SwsContext *img_convert_ctx;
    //struct SwsContext *sub_convert_ctx;
    int eof;

    char *filename;
    int width, height, xleft, ytop;
    int step;

#if CONFIG_AVFILTER
    int vfilter_idx;
    AVFilterContext *in_video_filter;   // the first filter in the video chain
    AVFilterContext *out_video_filter;  // the last filter in the video chain
    AVFilterContext *in_audio_filter;   // the first filter in the audio chain
    AVFilterContext *out_audio_filter;  // the last filter in the audio chain
    AVFilterGraph *agraph;              // audio filter graph
#endif

    int last_video_stream, last_audio_stream, last_subtitle_stream;

    pthread_cond_t continue_read_thread;


    AVPacket flush_pkt;

    int fast;
    int genpts;
    int lowres;

    int framedrop; // OPT_BOOL | OPT_EXPERT, { &framedrop }, "drop frames when cpu is too slow"
    int decoder_reorder_pts;
    int show_status;
    int loop;
    int autoexit;

    int seek_by_bytes;  //seek by bytes 0=off 1=on -1=auto
    int seek_interval;


    int64_t start_time;
    int64_t duration;

    const char* wanted_stream_spec[AVMEDIA_TYPE_NB];

    int display_disable;
    int audio_disable;
    int video_disable;
    int subtitle_disable;

    int infinite_buffer;

    double rdftspeed;
    int64_t audio_callback_time;

    const char *audio_codec_name;
    const char *subtitle_codec_name;
    const char *video_codec_name;

    AVDictionary *format_opts, *codec_opts, *resample_opts;


    //android
    void *surface;
    ANativeWindow *window;
    int window_format;
    uint8_t *pixels[4]; //sw
    int pitch[4];
    int image_size;

} VideoState;


static const struct AndroidFormatEntry {
    enum AVPixelFormat format;
    int android_fmt;
} android_format_map[] = {
        { AV_PIX_FMT_RGBA,          WINDOW_FORMAT_RGBA_8888 },
        { AV_PIX_FMT_RGBA,          WINDOW_FORMAT_RGBX_8888 },
        { AV_PIX_FMT_RGB565,         WINDOW_FORMAT_RGB_565 }
};

static void print_error(const char *filename, int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;

    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));
    av_log(NULL, AV_LOG_ERROR, "%s: %s\n", filename, errbuf_ptr);
}

static int is_realtime(AVFormatContext *s)
{
    if(   !strcmp(s->iformat->name, "rtp")
          || !strcmp(s->iformat->name, "rtsp")
          || !strcmp(s->iformat->name, "sdp")
            )
        return 1;

    if(s->pb && (   !strncmp(s->url, "rtp:", 4)
                    || !strncmp(s->url, "udp:", 4)
    )
            )
        return 1;
    return 0;
}


static JavaVM *java_vm;
static pthread_key_t current_env;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void jni_detach_env(void *data)
{
    if (java_vm) {
        (*java_vm).DetachCurrentThread();
    }
}

static void jni_create_pthread_key(void)
{
    pthread_key_create(&current_env, jni_detach_env);
}

JNIEnv *ff_jni_get_env(void *log_ctx)
{
    int ret = 0;
    JNIEnv *env = NULL;

    pthread_mutex_lock(&lock);
    if (java_vm == NULL) {
        java_vm = static_cast<JavaVM *>(av_jni_get_java_vm(log_ctx));
    }

    if (!java_vm) {
        av_log(log_ctx, AV_LOG_ERROR, "No Java virtual machine has been registered\n");
        goto done;
    }

    pthread_once(&once, jni_create_pthread_key);

    if ((env = static_cast<JNIEnv *>(pthread_getspecific(current_env))) != NULL) {
        goto done;
    }

    ret = (*java_vm).GetEnv((void **)&env, JNI_VERSION_1_6);
    switch(ret) {
        case JNI_EDETACHED:
            if ((*java_vm).AttachCurrentThread(&env, NULL) != 0) {
                av_log(log_ctx, AV_LOG_ERROR, "Failed to attach the JNI environment to the current thread\n");
                env = NULL;
            } else {
                pthread_setspecific(current_env, env);
            }
            break;
        case JNI_OK:
            break;
        case JNI_EVERSION:
            av_log(log_ctx, AV_LOG_ERROR, "The specified JNI version is not supported\n");
            break;
        default:
            av_log(log_ctx, AV_LOG_ERROR, "Failed to get the JNI environment attached to this thread\n");
            break;
    }

    done:
    pthread_mutex_unlock(&lock);
    return env;
}


static int decode_interrupt_cb(void *ctx)
{
    VideoState *is = (VideoState *)ctx;
    return is->abort_request;
}

static int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
    return stream_id < 0 ||
           queue->abort_request ||
           (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
           queue->nb_packets > MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}


/**
 * frame_queue start
 * @param vp
 */
static void frame_queue_unref_item(Frame *vp)
{
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}

static int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last)
{
    int i;
    memset(f, 0, sizeof(FrameQueue));
    if ((i = pthread_mutex_init(&f->mutex, NULL)) != 0) {
        av_log(NULL, AV_LOG_FATAL, "pthread_mutex_init(): %s\n", strerror(i));
        return AVERROR(ENOMEM);
    }
    if ((i = pthread_cond_init(&f->cond, NULL)) != 0) {
        av_log(NULL, AV_LOG_FATAL, "pthread_cond_init(): %s\n", strerror(i));
        return AVERROR(ENOMEM);
    }
    f->pktq = pktq;
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (i = 0; i < f->max_size; i++)
        if (!(f->queue[i].frame = av_frame_alloc()))
            return AVERROR(ENOMEM);
    return 0;
}

static void frame_queue_destory(FrameQueue *f)
{
    int i;
    for (i = 0; i < f->max_size; i++) {
        Frame *vp = &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
    }
    pthread_mutex_destroy(&f->mutex);
    pthread_cond_destroy(&f->cond);
}

static void frame_queue_signal(FrameQueue *f)
{
    pthread_mutex_lock(&f->mutex);
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

static Frame *frame_queue_peek(FrameQueue *f)
{
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static Frame *frame_queue_peek_next(FrameQueue *f)
{
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

static Frame *frame_queue_peek_last(FrameQueue *f)
{
    return &f->queue[f->rindex];
}

static Frame *frame_queue_peek_writable(FrameQueue *f)
{
    /* wait until we have space to put a new frame */
    pthread_mutex_lock(&f->mutex);
    while (f->size >= f->max_size &&
           !f->pktq->abort_request) {
        pthread_cond_wait(&f->cond, &f->mutex);
    }
    pthread_mutex_unlock(&f->mutex);

    if (f->pktq->abort_request)
        return NULL;

    return &f->queue[f->windex];
}

static Frame *frame_queue_peek_readable(FrameQueue *f)
{
    /* wait until we have a readable a new frame */
    pthread_mutex_lock(&f->mutex);
    while (f->size - f->rindex_shown <= 0 &&
           !f->pktq->abort_request) {
        pthread_cond_wait(&f->cond, &f->mutex);
    }
    pthread_mutex_unlock(&f->mutex);

    if (f->pktq->abort_request)
        return NULL;

    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static void frame_queue_push(FrameQueue *f)
{
    if (++f->windex == f->max_size)
        f->windex = 0;
    pthread_mutex_lock(&f->mutex);
    f->size++;
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

static void frame_queue_next(FrameQueue *f)
{
    if (f->keep_last && !f->rindex_shown) {
        f->rindex_shown = 1;
        return;
    }
    frame_queue_unref_item(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size)
        f->rindex = 0;
    pthread_mutex_lock(&f->mutex);
    f->size--;
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

/* return the number of undisplayed frames in the queue */
static int frame_queue_nb_remaining(FrameQueue *f)
{
    return f->size - f->rindex_shown;
}

/* return last shown position */
static int64_t frame_queue_last_pos(FrameQueue *f)
{
    Frame *fp = &f->queue[f->rindex];
    if (f->rindex_shown && fp->serial == f->pktq->serial)
        return fp->pos;
    else
        return -1;
}

/**
 * frame_queue end
 * @param vp
 */

/**
 * packet_queue start
 * @param vp
 */
static int packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
{
    MyAVPacketList *pkt1;

    if (q->abort_request)
        return -1;

    pkt1 = (MyAVPacketList *)av_malloc(sizeof(MyAVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    if (pkt == &q->is->flush_pkt)
        q->serial++;
    pkt1->serial = q->serial;

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
    q->duration += pkt1->pkt.duration;
    /* XXX: should duplicate packet data in DV case */
    //SDL_CondSignal(q->cond);
    pthread_cond_signal(&q->cond);
    return 0;
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
    int ret;

    pthread_mutex_lock(&q->mutex);
    ret = packet_queue_put_private(q, pkt);
    pthread_mutex_unlock(&q->mutex);

    if (pkt != &q->is->flush_pkt && ret < 0)
        av_packet_unref(pkt);

    return ret;
}

static int packet_queue_put_nullpacket(PacketQueue *q, int stream_index)
{
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}

/* packet queue handling */
static int packet_queue_init(PacketQueue *q)
{
    int i;
    memset(q, 0, sizeof(PacketQueue));
    if ((i = pthread_mutex_init(&q->mutex, NULL)) != 0) {
        av_log(NULL, AV_LOG_FATAL, "pthread_mutex_init(): %s\n", strerror(i));
        return AVERROR(ENOMEM);
    }

    if ((i = pthread_cond_init(&q->cond, NULL)) != 0) {
        av_log(NULL, AV_LOG_FATAL, "pthread_cond_init(): %s\n", strerror(i));
        return AVERROR(ENOMEM);
    }
    q->abort_request = 1;
    return 0;
}

static void packet_queue_flush(PacketQueue *q)
{
    MyAVPacketList *pkt, *pkt1;

    pthread_mutex_lock(&q->mutex);
    for (pkt = q->first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
    pthread_mutex_unlock(&q->mutex);
}

static void packet_queue_destroy(PacketQueue *q)
{
    packet_queue_flush(q);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

static void packet_queue_abort(PacketQueue *q)
{
    pthread_mutex_lock(&q->mutex);

    q->abort_request = 1;

    pthread_cond_signal(&q->cond);

    pthread_mutex_unlock(&q->mutex);
}

static void packet_queue_start(PacketQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 0;
    packet_queue_put_private(q, &q->is->flush_pkt);
    pthread_mutex_unlock(&q->mutex);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial)
{
    MyAVPacketList *pkt1;
    int ret;

    pthread_mutex_lock(&q->mutex);

    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size + sizeof(*pkt1);
            q->duration -= pkt1->pkt.duration;
            *pkt = pkt1->pkt;
            if (serial)
                *serial = pkt1->serial;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&q->cond, &q->mutex);
        }
    }
    pthread_mutex_unlock(&q->mutex);
    return ret;
}
/**
 * packet_queue end
 * @param vp
 */


/**
 * event_queue start
 * @param vp
 */
static int event_queue_put_private(EventQueue *q, MyEvent *evt)
{
    MyEventList *evt1;

    if (q->abort_request)
        return -1;

    evt1 = (MyEventList *)av_malloc(sizeof(MyEventList));
    if (!evt1)
        return -1;
    evt1->evt = *evt;
    evt1->next = NULL;

    if (!q->last_event)
        q->first_event = evt1;
    else
        q->last_event->next = evt1;
    q->last_event = evt1;
    q->nb_events++;
    q->size += evt1->evt.size + sizeof(*evt1);
    pthread_cond_signal(&q->cond);
    return 0;
}


static int event_queue_put(EventQueue *q, MyEvent *evt)
{
    int ret;

    pthread_mutex_lock(&q->mutex);
    ret = event_queue_put_private(q, evt);
    pthread_mutex_unlock(&q->mutex);

    return ret;
}

/* packet queue handling */
static int event_queue_init(EventQueue *e)
{
    int i;
    memset(e, 0, sizeof(EventQueue));
    if ((i = pthread_mutex_init(&e->mutex, NULL)) != 0) {
        av_log(NULL, AV_LOG_FATAL, "pthread_mutex_init(): %s\n", strerror(i));
        return AVERROR(ENOMEM);
    }

    if ((i = pthread_cond_init(&e->cond, NULL)) != 0) {
        av_log(NULL, AV_LOG_FATAL, "pthread_cond_init(): %s\n", strerror(i));
        return AVERROR(ENOMEM);
    }
    e->abort_request = 1;
    return 0;
}

static void event_queue_flush(EventQueue *e)
{
    MyEventList *evt, *evt1;

    pthread_mutex_lock(&e->mutex);
    for (evt = e->first_event; evt; evt = evt1) {
        evt1 = evt->next;
        if (evt->evt.data)
            av_free(evt->evt.data);
        av_freep(evt);
    }
    e->last_event = NULL;
    e->first_event = NULL;
    e->nb_events = 0;
    e->size = 0;
    pthread_mutex_unlock(&e->mutex);
}

static void event_queue_destroy(EventQueue *e)
{
    event_queue_flush(e);
    pthread_mutex_destroy(&e->mutex);
    pthread_cond_destroy(&e->cond);
}

static void event_queue_abort(EventQueue *e)
{
    pthread_mutex_lock(&e->mutex);

    e->abort_request = 1;

    pthread_cond_signal(&e->cond);

    pthread_mutex_unlock(&e->mutex);
}

static void event_queue_start(EventQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 0;
    pthread_mutex_unlock(&q->mutex);
}

/* return < 0 if aborted, 0 if no event and > 0 if event.  block = 0 is break, = 1 wait event*/
static int event_queue_get(EventQueue *e, MyEvent *evt, int block)
{
    MyEventList *evt1;
    int ret;

    pthread_mutex_lock(&e->mutex);

    for (;;) {
        if (e->abort_request) {
            ret = -1;
            break;
        }

        evt1 = e->first_event;
        if (evt1) {
            e->first_event = evt1->next;
            if (!e->first_event)
                e->last_event = NULL;
            e->nb_events--;
            e->size -= evt1->evt.size + sizeof(*evt1);
            *evt = evt1->evt;
            av_free(evt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&e->cond, &e->mutex);
        }
    }
    pthread_mutex_unlock(&e->mutex);
    return ret;
}


static void looper_init(Looper *l, EventQueue *queue) {
    memset(l, 0, sizeof(Looper));
    l->queue = queue;
}
static int looper_start(Looper *l, void *(*fn)(void *), const char *thread_name, void* arg)
{
    int i;
    event_queue_start(l->queue);

    if ((i = pthread_create(&l->looper_tid, NULL, fn, arg)) != 0) {
        av_log(NULL, AV_LOG_ERROR, "pthread_create(): %s:%s\n", thread_name, strerror(i));
        return AVERROR(ENOMEM);
    }

    av_log(NULL, AV_LOG_DEBUG, "looper_start(): %s\n", thread_name);

    return 0;
}
static void looper_abort(Looper *l)
{
    event_queue_abort(l->queue);
    pthread_join(l->looper_tid, 0);
    l->looper_tid = NULL;
}
static void looper_destroy(Looper *l) {
    //av_packet_unref(&d->pkt);
    //avcodec_free_context(&d->avctx);
}

/**
 * event_queue end
 * @param vp
 */


/**
 * clock start
 * @param vp
 */
static double get_clock(Clock *c)
{
    if (*c->queue_serial != c->serial)
        return NAN;
    if (c->paused) {
        return c->pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
    }
}

static void set_clock_at(Clock *c, double pts, int serial, double time)
{
    c->pts = pts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

static void set_clock(Clock *c, double pts, int serial)
{
    double time = av_gettime_relative() / 1000000.0;
    set_clock_at(c, pts, serial, time);
}

static void set_clock_speed(Clock *c, double speed)
{
    set_clock(c, get_clock(c), c->serial);
    c->speed = speed;
}

static void init_clock(Clock *c, int *queue_serial)
{
    c->speed = 1.0;
    c->paused = 0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

static void sync_clock_to_slave(Clock *c, Clock *slave)
{
    double clock = get_clock(c);
    double slave_clock = get_clock(slave);
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
        set_clock(c, slave_clock, slave->serial);
}

static int get_master_sync_type(VideoState *is) {
    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (is->video_st)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_AUDIO_MASTER;
    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (is->audio_st)
            return AV_SYNC_AUDIO_MASTER;
        else
            return AV_SYNC_EXTERNAL_CLOCK;
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}
/* get the current master clock value */
static double get_master_clock(VideoState *is)
{
    double val;

    switch (get_master_sync_type(is)) {
        case AV_SYNC_VIDEO_MASTER:
            val = get_clock(&is->vidclk);
            break;
        case AV_SYNC_AUDIO_MASTER:
            val = get_clock(&is->audclk);
            break;
        default:
            val = get_clock(&is->extclk);
            break;
    }
    return val;
}
/**
 * clock end
 * @param vp
 */

/**
 * decoder start
 * @param vp
 */
static void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, pthread_cond_t empty_queue_cond) {
    memset(d, 0, sizeof(Decoder));
    d->avctx = avctx;
    d->queue = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts = AV_NOPTS_VALUE;
    d->pkt_serial = -1;
}
static int decoder_start(Decoder *d, void *(*fn)(void *), const char *thread_name, void* arg)
{
    int i;
    packet_queue_start(d->queue);

    if ((i = pthread_create(&d->decoder_tid, NULL, fn, arg)) != 0) {
        av_log(NULL, AV_LOG_ERROR, "pthread_create(): %s:%s\n", thread_name, strerror(i));
        return AVERROR(ENOMEM);
    }

    av_log(NULL, AV_LOG_DEBUG, "decoder_start(): %s\n", thread_name);

    return 0;
}
static void decoder_abort(Decoder *d, FrameQueue *fq)
{
    packet_queue_abort(d->queue);
    frame_queue_signal(fq);
    pthread_join(d->decoder_tid, 0);
    d->decoder_tid = NULL;
    packet_queue_flush(d->queue);
}
static void decoder_destroy(Decoder *d) {
    av_packet_unref(&d->pkt);
    avcodec_free_context(&d->avctx);
}
static int decoder_decode_frame(Decoder *d, AVFrame *frame, AVSubtitle *sub) {
    int ret = AVERROR(EAGAIN);

    for (;;) {
        AVPacket pkt;

        if (d->queue->serial == d->pkt_serial) {
            do {
                if (d->queue->abort_request)
                    return -1;

                switch (d->avctx->codec_type) {
                    case AVMEDIA_TYPE_VIDEO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0) {
                            if (d->is->decoder_reorder_pts == -1) {
                                frame->pts = frame->best_effort_timestamp;
                            } else if (!d->is->decoder_reorder_pts) {
                                frame->pts = frame->pkt_dts;
                            }
                        }
                        break;
                    case AVMEDIA_TYPE_AUDIO:
                        ret = avcodec_receive_frame(d->avctx, frame);

                        if (ret >= 0) {
                            AVRational tb = (AVRational){1, frame->sample_rate};
                            if (frame->pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(frame->pts, d->avctx->pkt_timebase, tb);
                            else if (d->next_pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                            if (frame->pts != AV_NOPTS_VALUE) {
                                d->next_pts = frame->pts + frame->nb_samples;
                                d->next_pts_tb = tb;
                            }
                        }
                        break;
                }
                if (ret == AVERROR_EOF) {
                    d->finished = d->pkt_serial;
                    avcodec_flush_buffers(d->avctx);
                    return 0;
                }
                if (ret >= 0)
                    return 1;
            } while (ret != AVERROR(EAGAIN));
        }

        do {

            //av_log(NULL, AV_LOG_DEBUG, "audio_thread decoder_decode_frame 4 nb_packets:%d, packet_pending:%d, queue_serial:%d, pkt_serial:%d\n",
            //       d->queue->nb_packets, d->packet_pending, d->queue->serial, d->pkt_serial);

            if (d->queue->nb_packets == 0)
                pthread_cond_signal(&d->empty_queue_cond);
            if (d->packet_pending) {
                av_packet_move_ref(&pkt, &d->pkt);
                d->packet_pending = 0;
            } else {
                if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0)
                    return -1;
            }
        } while (d->queue->serial != d->pkt_serial);

        if (pkt.data == d->is->flush_pkt.data) {
            avcodec_flush_buffers(d->avctx);
            d->finished = 0;
            d->next_pts = d->start_pts;
            d->next_pts_tb = d->start_pts_tb;
        } else {
            if (d->avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                int got_frame = 0;
                ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, &pkt);
                if (ret < 0) {
                    ret = AVERROR(EAGAIN);
                } else {
                    if (got_frame && !pkt.data) {
                        d->packet_pending = 1;
                        av_packet_move_ref(&d->pkt, &pkt);
                    }
                    ret = got_frame ? 0 : (pkt.data ? AVERROR(EAGAIN) : AVERROR_EOF);
                }
            } else {
                if (avcodec_send_packet(d->avctx, &pkt) == AVERROR(EAGAIN)) {
                    av_log(d->avctx, AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                    d->packet_pending = 1;
                    av_packet_move_ref(&d->pkt, &pkt);
                }
            }
            av_packet_unref(&pkt);
        }
    }
}
/**
 * decoder end
 * @param vp
 */


static int queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
    Frame *vp;

#if defined(DEBUG_SYNC)
    printf("frame_type=%c pts=%0.3f\n",
           av_get_picture_type_char(src_frame->pict_type), pts);
#endif

    if (!(vp = frame_queue_peek_writable(&is->pictq)))
        return -1;

    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = 0;

    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;

    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;

    //set_default_window_size(vp->width, vp->height, vp->sar);

    av_frame_move_ref(vp->frame, src_frame);
    frame_queue_push(&is->pictq);
    return 0;
}


static int get_video_frame(VideoState *is, AVFrame *frame)
{
    int got_picture;

    if ((got_picture = decoder_decode_frame(&is->viddec, frame, NULL)) < 0)
        return -1;

    if (got_picture) {
        double dpts = NAN;

        if (frame->pts != AV_NOPTS_VALUE)
            dpts = av_q2d(is->video_st->time_base) * frame->pts;

        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

        if (is->framedrop>0 || (is->framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) {
            if (frame->pts != AV_NOPTS_VALUE) {
                double diff = dpts - get_master_clock(is);
                if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
                    diff - is->frame_last_filter_delay < 0 &&
                    is->viddec.pkt_serial == is->vidclk.serial &&
                    is->videoq.nb_packets) {
                    is->frame_drops_early++;
                    av_frame_unref(frame);
                    got_picture = 0;
                }
            }
        }
    }

    return got_picture;
}

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
    int err = 0;
    AVBufferRef *hw_device_ctx = NULL;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        av_log(NULL, AV_LOG_WARNING,
               "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

#if 0
    AVHWDeviceContext *device_ctx = (AVHWDeviceContext*)(ctx->hw_device_ctx->data);
    if (device_ctx->type == AV_HWDEVICE_TYPE_MEDIACODEC) {
        if (device_ctx->hwctx) {
            AVMediaCodecDeviceContext *mediacodec_ctx = (AVMediaCodecDeviceContext *)device_ctx->hwctx;
            LOGV("set surface %p\n", mediacodec_ctx->surface);
            mediacodec_ctx->surface = g_surface;
            LOGV("set surface %p\n", mediacodec_ctx->surface);
        }
    }
#endif

    return err;
}

static void *audio_thread(void *arg) {
    VideoState *is = (VideoState *)arg;
    AVFrame *frame = av_frame_alloc();
    Frame *af;
#if CONFIG_AVFILTER
    int last_serial = -1;
    int64_t dec_channel_layout;
    int reconfigure;
#endif
    int got_frame = 0;
    AVRational tb;
    int ret = 0;

    av_log(NULL, AV_LOG_DEBUG, "audio_thread enter\n");

    if (!frame)
        return (void*)AVERROR(ENOMEM);

    do {
        if ((got_frame = decoder_decode_frame(&is->auddec, frame, NULL)) < 0)
            goto the_end;

        if (got_frame) {
            tb = (AVRational){1, frame->sample_rate};

#if CONFIG_AVFILTER
            dec_channel_layout = get_valid_channel_layout(frame->channel_layout, frame->channels);

                reconfigure =
                    cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.channels,
                                   frame->format, frame->channels)    ||
                    is->audio_filter_src.channel_layout != dec_channel_layout ||
                    is->audio_filter_src.freq           != frame->sample_rate ||
                    is->auddec.pkt_serial               != last_serial;

                if (reconfigure) {
                    char buf1[1024], buf2[1024];
                    av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->audio_filter_src.channel_layout);
                    av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);
                    av_log(NULL, AV_LOG_DEBUG,
                           "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
                           is->audio_filter_src.freq, is->audio_filter_src.channels, av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial,
                           frame->sample_rate, frame->channels, av_get_sample_fmt_name(frame->format), buf2, is->auddec.pkt_serial);

                    is->audio_filter_src.fmt            = frame->format;
                    is->audio_filter_src.channels       = frame->channels;
                    is->audio_filter_src.channel_layout = dec_channel_layout;
                    is->audio_filter_src.freq           = frame->sample_rate;
                    last_serial                         = is->auddec.pkt_serial;

                    if ((ret = configure_audio_filters(is, afilters, 1)) < 0)
                        goto the_end;
                }

            if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0)
                goto the_end;

            while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0)) >= 0) {
                tb = av_buffersink_get_time_base(is->out_audio_filter);
#endif
            if (!(af = frame_queue_peek_writable(&is->sampq)))
                goto the_end;

            af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            af->pos = frame->pkt_pos;
            af->serial = is->auddec.pkt_serial;
            af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});

            av_frame_move_ref(af->frame, frame);
            frame_queue_push(&is->sampq);

#if CONFIG_AVFILTER
            if (is->audioq.serial != is->auddec.pkt_serial)
                    break;
            }
            if (ret == AVERROR_EOF)
                is->auddec.finished = is->auddec.pkt_serial;
#endif

            android_ActivateCallback(is->audios);
        }
    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
    the_end:
#if CONFIG_AVFILTER
    avfilter_graph_free(&is->agraph);
#endif
    av_frame_free(&frame);
    av_log(NULL, AV_LOG_DEBUG, "audio_thread leave\n");
    return (void*)ret;
}
static void *video_thread(void *arg) {
    VideoState *is = (VideoState *)arg;
    AVFrame *frame = av_frame_alloc();
    double pts;
    double duration;
    int ret;
    AVRational tb = is->video_st->time_base;
    AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

#if CONFIG_AVFILTER
    AVFilterGraph *graph = NULL;
    AVFilterContext *filt_out = NULL, *filt_in = NULL;
    int last_w = 0;
    int last_h = 0;
    enum AVPixelFormat last_format = -2;
    int last_serial = -1;
    int last_vfilter_idx = 0;
#endif

    av_log(NULL, AV_LOG_DEBUG, "video_thread enter\n");

    if (!frame)
        return (void*)AVERROR(ENOMEM);

    for (;;) {
        if (is->paused)
            continue;
        ret = get_video_frame(is, frame);
        if (ret < 0)
            goto the_end;
        if (!ret)
            continue;

#if CONFIG_AVFILTER
        if (   last_w != frame->width
            || last_h != frame->height
            || last_format != frame->format
            || last_serial != is->viddec.pkt_serial
            || last_vfilter_idx != is->vfilter_idx) {
            av_log(NULL, AV_LOG_DEBUG,
                   "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n",
                   last_w, last_h,
                   (const char *)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial,
                   frame->width, frame->height,
                   (const char *)av_x_if_null(av_get_pix_fmt_name(frame->format), "none"), is->viddec.pkt_serial);
            avfilter_graph_free(&graph);
            graph = avfilter_graph_alloc();
            if (!graph) {
                ret = AVERROR(ENOMEM);
                goto the_end;
            }
            graph->nb_threads = filter_nbthreads;
            if ((ret = configure_video_filters(graph, is, vfilters_list ? vfilters_list[is->vfilter_idx] : NULL, frame)) < 0) {
                SDL_Event event;
                event.type = FF_QUIT_EVENT;
                event.user.data1 = is;
                SDL_PushEvent(&event);
                goto the_end;
            }
            filt_in  = is->in_video_filter;
            filt_out = is->out_video_filter;
            last_w = frame->width;
            last_h = frame->height;
            last_format = frame->format;
            last_serial = is->viddec.pkt_serial;
            last_vfilter_idx = is->vfilter_idx;
            frame_rate = av_buffersink_get_frame_rate(filt_out);
        }

        ret = av_buffersrc_add_frame(filt_in, frame);
        if (ret < 0)
            goto the_end;

        while (ret >= 0) {
            is->frame_last_returned_time = av_gettime_relative() / 1000000.0;

            ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
            if (ret < 0) {
                if (ret == AVERROR_EOF)
                    is->viddec.finished = is->viddec.pkt_serial;
                ret = 0;
                break;
            }

            is->frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
            if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
                is->frame_last_filter_delay = 0;
            tb = av_buffersink_get_time_base(filt_out);
#endif
#if 0
        AVMediaCodecBuffer *buffer = (AVMediaCodecBuffer *) frame->data[3];
        if (buffer)
            av_mediacodec_release_buffer(buffer, 1);

        av_usleep(22000);
#else
        duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        ret = queue_picture(is, frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
#endif
        av_frame_unref(frame);
#if CONFIG_AVFILTER
        if (is->videoq.serial != is->viddec.pkt_serial)
                break;
        }
#endif

        if (ret < 0)
            goto the_end;
    }
    the_end:
#if CONFIG_AVFILTER
    avfilter_graph_free(&graph);
#endif
    av_frame_free(&frame);
    av_log(NULL, AV_LOG_DEBUG, "video_thread leave\n");
    return 0;
}
static void *subtitle_thread(void *arg) {
    VideoState *is = (VideoState *)arg;
    Frame *sp;
    int got_subtitle;
    double pts;

    av_log(NULL, AV_LOG_DEBUG, "subtitle_thread enter\n");

    for (;;) {
        if (!(sp = frame_queue_peek_writable(&is->subpq)))
            return 0;

        if ((got_subtitle = decoder_decode_frame(&is->subdec, NULL, &sp->sub)) < 0)
            break;

        pts = 0;

        if (got_subtitle && sp->sub.format == 0) {
            if (sp->sub.pts != AV_NOPTS_VALUE)
                pts = sp->sub.pts / (double)AV_TIME_BASE;
            sp->pts = pts;
            sp->serial = is->subdec.pkt_serial;
            sp->width = is->subdec.avctx->width;
            sp->height = is->subdec.avctx->height;
            sp->uploaded = 0;

            /* now we can update the picture count */
            frame_queue_push(&is->subpq);
        } else if (got_subtitle) {
            avsubtitle_free(&sp->sub);
        }
    }

    av_log(NULL, AV_LOG_DEBUG, "subtitle_thread leave\n");
    return 0;
}

#define SAMPLERATE 44100
#define CHANNELS 1
#define PERIOD_TIME 20 //ms
#define FRAME_SIZE SAMPLERATE*PERIOD_TIME/1000


/* copy samples for viewing in editor window */
static void update_sample_display(VideoState *is, short *samples, int samples_size)
{
    int size, len;

    size = samples_size / sizeof(short);
    while (size > 0) {
        len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
        if (len > size)
            len = size;
        memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
        samples += len;
        is->sample_array_index += len;
        if (is->sample_array_index >= SAMPLE_ARRAY_SIZE)
            is->sample_array_index = 0;
        size -= len;
    }
}

/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
static int synchronize_audio(VideoState *is, int nb_samples)
{
    int wanted_nb_samples = nb_samples;

    /* if not master, then we try to remove or add samples to correct the clock */
    if (get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER) {
        double diff, avg_diff;
        int min_nb_samples, max_nb_samples;

        diff = get_clock(&is->audclk) - get_master_clock(is);

        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
            is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
            if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
                /* not enough measures to have a correct estimate */
                is->audio_diff_avg_count++;
            } else {
                /* estimate the A-V difference */
                avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

                if (fabs(avg_diff) >= is->audio_diff_threshold) {
                    wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.freq);
                    min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
                }
                av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
                       diff, avg_diff, wanted_nb_samples - nb_samples,
                       is->audio_clock, is->audio_diff_threshold);
            }
        } else {
            /* too big difference : may be initial PTS errors, so
               reset A-V filter */
            is->audio_diff_avg_count = 0;
            is->audio_diff_cum       = 0;
        }
    }

    return wanted_nb_samples;
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
static int audio_decode_frame(VideoState *is)
{
    int data_size, resampled_data_size;
    int64_t dec_channel_layout;
    av_unused double audio_clock0;
    int wanted_nb_samples;
    Frame *af;

    if (is->paused)
        return -1;

    do {
#if defined(_WIN32)
        while (frame_queue_nb_remaining(&is->sampq) == 0) {
            if ((av_gettime_relative() - audio_callback_time) > 1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2)
                return -1;
            av_usleep (1000);
        }
#endif
        if (!(af = frame_queue_peek_readable(&is->sampq)))
            return -1;
        frame_queue_next(&is->sampq);
    } while (af->serial != is->audioq.serial);

    data_size = av_samples_get_buffer_size(NULL, af->frame->channels,
                                           af->frame->nb_samples,
                                           static_cast<AVSampleFormat>(af->frame->format), 1);

    dec_channel_layout =
            (af->frame->channel_layout && af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ?
            af->frame->channel_layout : av_get_default_channel_layout(af->frame->channels);
    wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);

    if (af->frame->format        != is->audio_src.fmt            ||
        dec_channel_layout       != is->audio_src.channel_layout ||
        af->frame->sample_rate   != is->audio_src.freq           ||
        (wanted_nb_samples       != af->frame->nb_samples && !is->swr_ctx)) {
        swr_free(&is->swr_ctx);
        is->swr_ctx = swr_alloc_set_opts(NULL,
                                         is->audio_tgt.channel_layout, is->audio_tgt.fmt, is->audio_tgt.freq,
                                         dec_channel_layout,
                                         static_cast<AVSampleFormat>(af->frame->format), af->frame->sample_rate,
                                         0, NULL);
        if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                   af->frame->sample_rate, av_get_sample_fmt_name(
                            static_cast<AVSampleFormat>(af->frame->format)), af->frame->channels,
                   is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
            swr_free(&is->swr_ctx);
            return -1;
        }
        is->audio_src.channel_layout = dec_channel_layout;
        is->audio_src.channels       = af->frame->channels;
        is->audio_src.freq = af->frame->sample_rate;
        is->audio_src.fmt = static_cast<AVSampleFormat>(af->frame->format);
    }

    if (is->swr_ctx) {
        const uint8_t **in = (const uint8_t **)af->frame->extended_data;
        uint8_t **out = &is->audio_buf1;
        int out_count = (int64_t)wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256;
        int out_size  = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0) {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (wanted_nb_samples != af->frame->nb_samples) {
            if (swr_set_compensation(is->swr_ctx, (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate,
                                     wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) < 0) {
                av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                return -1;
            }
        }
        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
        if (!is->audio_buf1)
            return AVERROR(ENOMEM);
        len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0) {
            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
            return -1;
        }
        if (len2 == out_count) {
            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
            if (swr_init(is->swr_ctx) < 0)
                swr_free(&is->swr_ctx);
        }
        is->audio_buf = is->audio_buf1;
        resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
    } else {
        is->audio_buf = af->frame->data[0];
        resampled_data_size = data_size;
    }

    audio_clock0 = is->audio_clock;
    /* update the audio clock with the pts */
    if (!isnan(af->pts))
        is->audio_clock = af->pts + (double) af->frame->nb_samples / af->frame->sample_rate;
    else
        is->audio_clock = NAN;
    is->audio_clock_serial = af->serial;
#ifdef DEBUG
    {
        static double last_clock;
        printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
               is->audio_clock - last_clock,
               is->audio_clock, audio_clock0);
        last_clock = is->audio_clock;
    }
#endif
    return resampled_data_size;
}

/* prepare a new audio buffer */
static void sdl_audio_callback(VideoState *opaque, uint8_t *stream, int len)
{
    VideoState *is = opaque;
    int audio_size, len1;

    is->audio_callback_time = av_gettime_relative();

    while (len > 0) {
        if (is->audio_buf_index >= is->audio_buf_size) {
            audio_size = audio_decode_frame(is);
            if (audio_size < 0) {
                /* if error, just output silence */
                is->audio_buf = NULL;
                is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
            } else {
                if (is->show_mode != VideoState::SHOW_MODE_VIDEO)
                    update_sample_display(is, (int16_t *)is->audio_buf, audio_size);
                is->audio_buf_size = audio_size;
            }
            is->audio_buf_index = 0;
        }
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len)
            len1 = len;

        if (!is->muted && is->audio_buf) // && is->audio_volume == SDL_MIX_MAXVOLUME
            memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
        else {
            //memset(stream, 0, len1);
            //if (!is->muted && is->audio_buf)
            //    SDL_MixAudioFormat(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, is->audio_volume);
        }
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!isnan(is->audio_clock)) {
        set_clock_at(&is->audclk, is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec, is->audio_clock_serial, is->audio_callback_time / 1000000.0);
        sync_clock_to_slave(&is->extclk, &is->audclk);
    }
}

void sl_audio_callback(
        OPENSL_STREAM *p,
        void *pContext
)
{
    VideoState *is = (VideoState *)pContext;

    av_log(NULL, AV_LOG_DEBUG, "sl_audio_callback %d\n", p->outBufSamples);

    short *outBuffer = p->outputBuffer[p->currentOutputBuffer];
    int bufsamps = p->outBufSamples;

    sdl_audio_callback(is, reinterpret_cast<uint8_t *>(outBuffer), bufsamps);

    (*p->bqPlayerBufferQueue)->Enqueue(p->bqPlayerBufferQueue,
                                       outBuffer,bufsamps*sizeof(short));
}

static int audio_open(VideoState *is, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams *audio_hw_params)
{
    //freq 指定了每秒向音频设备发送的 sample 数。常用的值为：11025，22050，44100。值越高质量越好。
    //format 指定了每个 sample 元素的大小和类型。可能取值如下：
    //channels 指定了声音的通道数：1（单声道）2（立体声）。
    //samples 这个值表示音频缓存区的大小（以 sample 计）。一个 sample 是一段大小为 format * channels 的音频数据。
    //size 这个值表示音频缓存区的大小（以 byte 计）。

#if 1
    static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;


    av_log(NULL, AV_LOG_DEBUG, "audio is wanted_channel_layout:%d, wanted_nb_channels:%d, wanted_sample_rate:%d, \n",
           wanted_channel_layout, wanted_nb_channels, wanted_sample_rate);

    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }

    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    int channels = wanted_nb_channels;

    int freq = wanted_sample_rate;
    if (freq <= 0 || channels <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    //while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= freq)
    //    next_sample_rate_idx--;

    //wanted_spec.format = AUDIO_S16SYS;
    //wanted_spec.silence = 0;
    int samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    //wanted_spec.callback = sdl_audio_callback;
    //wanted_spec.userdata = opaque;

#if 0
    OPENSL_STREAM* stream = android_OpenAudioDevice(freq, 0, channels, FRAME_SIZE);
    if (stream == NULL) {
        av_log(NULL, AV_LOG_ERROR, "failed to open audio device ! \n");
        return -1;
    }
#endif

    audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = freq;
    audio_hw_params->channel_layout = wanted_channel_layout;
    audio_hw_params->channels =  channels;
    audio_hw_params->frame_size = FRAME_SIZE; //av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
        //android_CloseAudioDevice(stream);
        return -1;
    }

#if 1
    OPENSL_STREAM* stream = android_OpenAudioDevice(audio_hw_params->freq, 0, audio_hw_params->channels, audio_hw_params->frame_size);
    if (stream == NULL) {
        av_log(NULL, AV_LOG_ERROR, "failed to open audio device ! \n");
        return -1;
    }

    av_log(NULL, AV_LOG_VERBOSE, "open audio device ! freq:%d, channels:%d, frame_size:%d\n",
           audio_hw_params->freq, audio_hw_params->channels, audio_hw_params->frame_size);

    android_RegisterCallback(stream, sl_audio_callback, is);

    is->audios = stream;
    //return spec.size;
    return stream->outBufSamples;

#else

    return 1;

#endif


#else
    SDL_AudioSpec wanted_spec, spec;
    const char *env;
    static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

    env = SDL_getenv("SDL_AUDIO_CHANNELS");
    if (env) {
        wanted_nb_channels = atoi(env);
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
    }
    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;
    wanted_spec.freq = wanted_sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
        next_sample_rate_idx--;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = sdl_audio_callback;
    wanted_spec.userdata = opaque;
    while (!(audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE))) {
        av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
               wanted_spec.channels, wanted_spec.freq, SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                av_log(NULL, AV_LOG_ERROR,
                       "No more combinations to try, audio open failed\n");
                return -1;
            }
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }
    if (spec.format != AUDIO_S16SYS) {
        av_log(NULL, AV_LOG_ERROR,
               "SDL advised audio format %d is not supported!\n", spec.format);
        return -1;
    }
    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            av_log(NULL, AV_LOG_ERROR,
                   "SDL advised channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }

    audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = spec.freq;
    audio_hw_params->channel_layout = wanted_channel_layout;
    audio_hw_params->channels =  spec.channels;
    audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
        return -1;
    }
    return spec.size;
#endif
}

/* open a given stream. Return 0 if OK */
static int stream_component_open(VideoState *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx;
    const AVCodec *codec;
    const char *forced_codec_name = NULL;
    AVDictionary *opts = NULL;
    AVDictionaryEntry *t = NULL;
    int sample_rate, nb_channels;
    int64_t channel_layout;
    int ret = 0;
    int stream_lowres = is->lowres;
    enum AVHWDeviceType hw_type;
    AVMediaCodecContext *media_codec_ctx = NULL;

    av_log(NULL, AV_LOG_DEBUG, "stream_component_open stream_index:%d\n", stream_index);

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;

    if (ic->streams[stream_index]->codecpar->width == 0 && ic->streams[stream_index]->codecpar->height == 0) {
        ic->streams[stream_index]->codecpar->width = 1920;
        ic->streams[stream_index]->codecpar->height = 1080;
    }

    avctx = avcodec_alloc_context3(NULL);
    if (!avctx)
        return AVERROR(ENOMEM);
#if 0
    if (ic->streams[stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
        if (!ic->streams[stream_index]->codecpar->extradata)
        {
            //unsigned char sps_pps[23] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x0f, 0x00, 0x44, 0xbe, 0x8,
            //                              0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };

            unsigned char data[23] = {
                    0x00, 0x00, 0x00, 0x01, 0x67, 0x4D, 0x00, 0x2A, 0x95, 0xA8, 0x1E, 0x00, 0x89,
                    0xF9, 0x50, 0x00,
                    0x00, 0x00, 0x01, 0x68, 0xEE, 0x3C, 0x80
            };

            ic->streams[stream_index]->codecpar->extradata_size = 23;
            ic->streams[stream_index]->codecpar->extradata = (uint8_t *) av_malloc(
                    ic->streams[stream_index]->codecpar->extradata_size);
            if (ic->streams[stream_index]->codecpar->extradata) {
                memcpy(ic->streams[stream_index]->codecpar->extradata, data, ic->streams[stream_index]->codecpar->extradata_size);
            }
        }

        if (is->surface)
        {
            hw_type = av_hwdevice_find_type_by_name("mediacodec");
            if (hw_type == AV_HWDEVICE_TYPE_NONE) {
                av_log(NULL, AV_LOG_WARNING,
                       "No hwdevice could be found with name '%s'\n", "mediacodec");
                goto fail;
            }

            media_codec_ctx = av_mediacodec_alloc_context();
            if (!media_codec_ctx) {
                av_log(NULL, AV_LOG_WARNING,
                       "Could not allocate mediacodec context\n");
                goto fail;
            }

            if ((ret = av_mediacodec_default_init(avctx, media_codec_ctx, is->surface)) < 0) {
                av_log(NULL, AV_LOG_WARNING,
                       "Failed to init mediacodec, surface:%x,  ret:%d\n", is->surface, ret);
                goto fail;
            }

            if ((ret = hw_decoder_init(avctx, hw_type)) < 0){
                av_log(NULL, AV_LOG_WARNING,
                       "Failed to init hw_decoder, surface:%x,  ret:%d\n", is->surface, ret);
                goto fail;
            }
        }
    }
#endif

    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0)
        goto fail;
    avctx->pkt_timebase = ic->streams[stream_index]->time_base;


    av_log(NULL, AV_LOG_DEBUG, "stream_component_open stream_index:%d, codec_id:%d\n", stream_index, avctx->codec_id);

    codec = avcodec_find_decoder(avctx->codec_id);

    switch(avctx->codec_type){
        case AVMEDIA_TYPE_AUDIO   : is->last_audio_stream    = stream_index; forced_codec_name =    is->audio_codec_name; break;
        case AVMEDIA_TYPE_SUBTITLE: is->last_subtitle_stream = stream_index; forced_codec_name =    is->subtitle_codec_name; break;
        case AVMEDIA_TYPE_VIDEO   : is->last_video_stream    = stream_index; forced_codec_name =    is->video_codec_name; break;
    }
    if (forced_codec_name)
        codec = avcodec_find_decoder_by_name(forced_codec_name);
    if (!codec) {
        if (forced_codec_name) av_log(NULL, AV_LOG_WARNING,
                                      "No codec could be found with name '%s'\n", forced_codec_name);
        else                   av_log(NULL, AV_LOG_WARNING,
                                      "No decoder could be found for codec %s\n", avcodec_get_name(avctx->codec_id));
        ret = AVERROR(EINVAL);
        goto fail;
    }


    av_log(NULL, AV_LOG_DEBUG, "stream_component_open stream_index:%d, forced_codec_name:%s, codec_name:%s\n", stream_index, forced_codec_name, codec->name);

    avctx->codec_id = codec->id;
    if (stream_lowres > codec->max_lowres) {
        av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
               codec->max_lowres);
        stream_lowres = codec->max_lowres;
    }
    avctx->lowres = stream_lowres;

    if (is->fast)
        avctx->flags2 |= AV_CODEC_FLAG2_FAST;

    //opts = filter_codec_opts(is->codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
    if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    if (stream_lowres)
        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
        av_log(NULL, AV_LOG_FATAL, "avcodec_open2() failed\n");
        goto fail;
    }
#if 0
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        ret =  AVERROR_OPTION_NOT_FOUND;
        goto fail;
    }
#endif

    av_log(NULL, AV_LOG_DEBUG, "stream_component_open stream_index:%d open success\n", stream_index);

    is->eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
#if CONFIG_AVFILTER
            {
            AVFilterContext *sink;

            is->audio_filter_src.freq           = avctx->sample_rate;
            is->audio_filter_src.channels       = avctx->channels;
            is->audio_filter_src.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
            is->audio_filter_src.fmt            = avctx->sample_fmt;
            if ((ret = configure_audio_filters(is, afilters, 0)) < 0)
                goto fail;
            sink = is->out_audio_filter;
            sample_rate    = av_buffersink_get_sample_rate(sink);
            nb_channels    = av_buffersink_get_channels(sink);
            channel_layout = av_buffersink_get_channel_layout(sink);
        }
#else
            sample_rate    = avctx->sample_rate;
            nb_channels    = avctx->channels;
            channel_layout = avctx->channel_layout;
#endif

#if 1
            /* prepare audio output */
            if ((ret = audio_open(is, channel_layout, nb_channels, sample_rate, &is->audio_tgt)) < 0)
                goto fail;
            is->audio_hw_buf_size = ret;
            is->audio_src = is->audio_tgt;
            is->audio_buf_size  = 0;
            is->audio_buf_index = 0;

            /* init averaging filter */
            is->audio_diff_avg_coef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
            is->audio_diff_avg_count = 0;
            /* since we do not have a precise anough audio FIFO fullness,
               we correct audio sync only if larger than this threshold */
            is->audio_diff_threshold = (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;

            is->audio_stream = stream_index;
            is->audio_st = ic->streams[stream_index];


            decoder_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread);
            if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !is->ic->iformat->read_seek) {
                is->auddec.start_pts = is->audio_st->start_time;
                is->auddec.start_pts_tb = is->audio_st->time_base;
            }

            is->auddec.is = is;

            if ((ret = decoder_start(&is->auddec, audio_thread, "audio_decoder", is)) < 0)
                goto out;
            //SDL_PauseAudioDevice(audio_dev, 0);
            android_Play(is->audios);
            av_log(NULL, AV_LOG_DEBUG, "audio play successed\n");
#endif
            break;
        case AVMEDIA_TYPE_VIDEO:
            is->video_stream = stream_index;
            is->video_st = ic->streams[stream_index];

            if (is->surface) {
                JNIEnv *env = ff_jni_get_env(is->viddec.avctx);
                if (!env) {
                    av_log(NULL, AV_LOG_FATAL, "jni_get_env failed\n");
                    ret = -1;
                    goto out;
                }

                is->window = ANativeWindow_fromSurface(env, (jobject)is->surface);

                av_log(NULL, AV_LOG_INFO, "ANativeWindow_fromSurface window: %x, surface: %x\n", is->window, is->surface);
            }

            decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);
            is->viddec.is = is;
            if ((ret = decoder_start(&is->viddec, video_thread, "video_decoder", is)) < 0)
                goto out;
            is->queue_attachments_req = 1;
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            is->subtitle_stream = stream_index;
            is->subtitle_st = ic->streams[stream_index];

            decoder_init(&is->subdec, avctx, &is->subtitleq, is->continue_read_thread);
            is->subdec.is = is;
            if ((ret = decoder_start(&is->subdec, subtitle_thread, "subtitle_decoder", is)) < 0)
                goto out;
            break;
        default:
            break;
    }
    goto out;

fail:
    av_mediacodec_default_free(avctx);
    avcodec_free_context(&avctx);
out:
    av_dict_free(&opts);

    return ret;
}

static void stream_component_close(VideoState *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecParameters *codecpar;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return;
    codecpar = ic->streams[stream_index]->codecpar;

    switch (codecpar->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
#if 0
            decoder_abort(&is->auddec, &is->sampq);
            SDL_CloseAudioDevice(audio_dev);
            decoder_destroy(&is->auddec);
            swr_free(&is->swr_ctx);
            av_freep(&is->audio_buf1);
            is->audio_buf1_size = 0;
            is->audio_buf = NULL;

            if (is->rdft) {
                av_rdft_end(is->rdft);
                av_freep(&is->rdft_data);
                is->rdft = NULL;
                is->rdft_bits = 0;
            }
#endif
            break;
        case AVMEDIA_TYPE_VIDEO:
            decoder_abort(&is->viddec, &is->pictq);
            decoder_destroy(&is->viddec);
            if (is->window)
                ANativeWindow_release(is->window);
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            decoder_abort(&is->subdec, &is->subpq);
            decoder_destroy(&is->subdec);
            break;
        default:
            break;
    }

    ic->streams[stream_index]->discard = AVDISCARD_ALL;
    switch (codecpar->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            is->audio_st = NULL;
            is->audio_stream = -1;
            break;
        case AVMEDIA_TYPE_VIDEO:
            is->video_st = NULL;
            is->video_stream = -1;
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            is->subtitle_st = NULL;
            is->subtitle_stream = -1;
            break;
        default:
            break;
    }
}

static void check_external_clock_speed(VideoState *is) {
    if (is->video_stream >= 0 && is->videoq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES ||
        is->audio_stream >= 0 && is->audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES) {
        set_clock_speed(&is->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, is->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
    } else if ((is->video_stream < 0 || is->videoq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
               (is->audio_stream < 0 || is->audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)) {
        set_clock_speed(&is->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, is->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
    } else {
        double speed = is->extclk.speed;
        if (speed != 1.0)
            set_clock_speed(&is->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
    }
}

/* seek in the stream */
static void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes)
{
    if (!is->seek_req) {
        is->seek_pos = pos;
        is->seek_rel = rel;
        is->seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes)
            is->seek_flags |= AVSEEK_FLAG_BYTE;
        is->seek_req = 1;
        pthread_cond_signal(&is->continue_read_thread);
    }
}

/* pause or resume the video */
static void stream_toggle_pause(VideoState *is)
{
    if (is->paused) {
        is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
        if (is->read_pause_return != AVERROR(ENOSYS)) {
            is->vidclk.paused = 0;
        }
        set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
    }
    set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
    is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;
}

static void seek_chapter(VideoState *is, int incr)
{
    int64_t pos = get_master_clock(is) * AV_TIME_BASE;
    int i;

    if (!is->ic->nb_chapters)
        return;

    /* find the current chapter */
    for (i = 0; i < is->ic->nb_chapters; i++) {
        AVChapter *ch = is->ic->chapters[i];
        if (av_compare_ts(pos, AV_TIME_BASE_Q, ch->start, ch->time_base) < 0) {
            i--;
            break;
        }
    }

    i += incr;
    i = FFMAX(i, 0);
    if (i >= is->ic->nb_chapters)
        return;

    av_log(NULL, AV_LOG_VERBOSE, "Seeking to chapter %d.\n", i);
    stream_seek(is, av_rescale_q(is->ic->chapters[i]->start, is->ic->chapters[i]->time_base,
                                 AV_TIME_BASE_Q), 0, 0);
}

static void step_to_next_frame(VideoState *is)
{
    /* if the stream is paused unpause it, then step */
    if (is->paused)
        stream_toggle_pause(is);
    is->step = 1;
}

static void toggle_pause(VideoState *is)
{
    stream_toggle_pause(is);
    is->step = 0;
}

static void toggle_mute(VideoState *is)
{
    is->muted = !is->muted;
}

static void update_volume(VideoState *is, int sign, double step)
{
    double volume_level = is->audio_volume ? (20 * log(is->audio_volume / (double)SDL_MIX_MAXVOLUME) / log(10)) : -1000.0;
    int new_volume = lrint(SDL_MIX_MAXVOLUME * pow(10.0, (volume_level + sign * step) / 20.0));
    is->audio_volume = av_clip(is->audio_volume == new_volume ? (is->audio_volume + sign) : new_volume, 0, SDL_MIX_MAXVOLUME);
}

/* this thread gets the stream from the disk or the network */
static void *read_thread(void *arg) {
    VideoState *is = (VideoState *)arg;
    AVFormatContext *ic = NULL;
    int err, i, ret;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket pkt1, *pkt = &pkt1;
    int64_t stream_start_time;
    int pkt_in_play_range = 0;
    AVDictionaryEntry *t;
    pthread_mutex_t wait_mutex;
    int scan_all_pmts_set = 0;
    int64_t pkt_ts;
    AVDictionary *format_opts;
    struct timeval now;
    struct timespec timeout;

    av_log(NULL, AV_LOG_DEBUG, "read_thread enter\n");

    if ((ret = pthread_mutex_init(&wait_mutex, NULL)) != 0) {
        av_log(NULL, AV_LOG_FATAL, "pthread_mutex_init(): %s\n", strerror(ret));
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    memset(st_index, -1, sizeof(st_index));
    is->last_video_stream = is->video_stream = -1;
    is->last_audio_stream = is->audio_stream = -1;
    is->last_subtitle_stream = is->subtitle_stream = -1;
    is->eof = 0;

    ic = avformat_alloc_context();
    if (!ic) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = is;
#if 0
    if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
        av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }
    err = avformat_open_input(&ic, is->filename, is->iformat, &format_opts);
#else

    av_dict_set(&format_opts,"rtsp_transport","tcp",0);

    err = avformat_open_input(&ic, is->filename, is->iformat, &format_opts);
#endif
    if (err < 0) {
        print_error(is->filename, err);
        ret = -1;
        goto fail;
    }
    is->ic = ic;

    av_log(NULL, AV_LOG_DEBUG, "avformat_open_input success\n");

    if (is->genpts)
        ic->flags |= AVFMT_FLAG_GENPTS;

    av_format_inject_global_side_data(ic);

    if (ic->pb)
        ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

    if (is->seek_by_bytes < 0)
        is->seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);

    is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    /* if seeking requested, we execute it */
    if (is->start_time != AV_NOPTS_VALUE) {
        int64_t timestamp;

        timestamp = is->start_time;
        /* add the stream start time */
        if (ic->start_time != AV_NOPTS_VALUE)
            timestamp += ic->start_time;
        ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                   is->filename, (double)timestamp / AV_TIME_BASE);
        }
    }

    is->realtime = is_realtime(ic);

    if (is->show_status)
        av_dump_format(ic, 0, is->filename, 0);

    for (i = 0; i < ic->nb_streams; i++) {
        AVStream *st = ic->streams[i];
        enum AVMediaType type = st->codecpar->codec_type;
        st->discard = AVDISCARD_ALL;
        if (type >= 0 && is->wanted_stream_spec[type] && st_index[type] == -1)
            if (avformat_match_stream_specifier(ic, st, is->wanted_stream_spec[type]) > 0)
                st_index[type] = i;
    }
    for (i = 0; i < AVMEDIA_TYPE_NB; i++) {
        if (is->wanted_stream_spec[i] && st_index[i] == -1) {
            av_log(NULL, AV_LOG_ERROR, "Stream specifier %s does not match any %s stream\n", is->wanted_stream_spec[i], av_get_media_type_string((enum AVMediaType)i));
            st_index[i] = INT_MAX;
        }
    }

    if (!is->video_disable)
        st_index[AVMEDIA_TYPE_VIDEO] =
                av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
                                    st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
    if (!is->audio_disable)
        st_index[AVMEDIA_TYPE_AUDIO] =
                av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
                                    st_index[AVMEDIA_TYPE_AUDIO],
                                    st_index[AVMEDIA_TYPE_VIDEO],
                                    NULL, 0);
    if (!is->video_disable && !is->subtitle_disable)
        st_index[AVMEDIA_TYPE_SUBTITLE] =
                av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
                                    st_index[AVMEDIA_TYPE_SUBTITLE],
                                    (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
                                     st_index[AVMEDIA_TYPE_AUDIO] :
                                     st_index[AVMEDIA_TYPE_VIDEO]),
                                    NULL, 0);

    av_log(NULL, AV_LOG_DEBUG, "av_find_best_stream AVMEDIA_TYPE_VIDEO:%d, AVMEDIA_TYPE_AUDIO:%d, AVMEDIA_TYPE_SUBTITLE:%d\n",
            st_index[AVMEDIA_TYPE_VIDEO], st_index[AVMEDIA_TYPE_AUDIO], st_index[AVMEDIA_TYPE_SUBTITLE]);

    is->show_mode = VideoState::SHOW_MODE_NONE;

    /* open the streams */
    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
    }

    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
    }
    if (is->show_mode == VideoState::SHOW_MODE_NONE)
        is->show_mode = ret >= 0 ? VideoState::SHOW_MODE_VIDEO : VideoState::SHOW_MODE_RDFT;

    if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
        stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
    }

    if (is->video_stream < 0 && is->audio_stream < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n",
               is->filename);
        ret = -1;
        goto fail;
    }

    if (is->infinite_buffer < 0 && is->realtime)
        is->infinite_buffer = 1;

    for (;;) {
        if (is->abort_request)
            break;
        if (is->paused != is->last_paused) {
            av_log(NULL, AV_LOG_DEBUG,"paused loop:%d, read:%d\n", is->loop, ret);
            is->last_paused = is->paused;
            if (is->paused)
                is->read_pause_return = av_read_pause(ic);
            else
                av_read_play(ic);
        }
        if (is->paused &&
            (!strcmp(ic->iformat->name, "rtsp") ||
             (ic->pb && !strncmp(is->filename, "mmsh:", 5)))) {
            /* wait 10 ms to avoid trying to get another packet */
            /* XXX: horrible */
            //SDL_Delay(10);
            av_log(NULL, AV_LOG_DEBUG,"wait 1 loop:%d, read:%d\n", is->loop, ret);
            av_usleep(10000);
            continue;
        }

        if (is->seek_req) {
            int64_t seek_target = is->seek_pos;
            int64_t seek_min    = is->seek_rel > 0 ? seek_target - is->seek_rel + 2: INT64_MIN;
            int64_t seek_max    = is->seek_rel < 0 ? seek_target - is->seek_rel - 2: INT64_MAX;
// FIXME the +-2 is due to rounding being not done in the correct direction in generation
//      of the seek_pos/seek_rel variables

            ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR,
                       "%s: error while seeking\n", is->ic->url);
            } else {
                if (is->audio_stream >= 0) {
                    packet_queue_flush(&is->audioq);
                    packet_queue_put(&is->audioq, &is->flush_pkt);
                }
                if (is->subtitle_stream >= 0) {
                    packet_queue_flush(&is->subtitleq);
                    packet_queue_put(&is->subtitleq, &is->flush_pkt);
                }
                if (is->video_stream >= 0) {
                    packet_queue_flush(&is->videoq);
                    packet_queue_put(&is->videoq, &is->flush_pkt);
                }
                if (is->seek_flags & AVSEEK_FLAG_BYTE) {
                    set_clock(&is->extclk, NAN, 0);
                } else {
                    set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
                }
            }
            is->seek_req = 0;
            is->queue_attachments_req = 1;
            is->eof = 0;
            if (is->paused)
                step_to_next_frame(is);
        }

        if (is->queue_attachments_req) {
            if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                AVPacket copy = { 0 };
                if ((ret = av_packet_ref(&copy, &is->video_st->attached_pic)) < 0)
                    goto fail;
                packet_queue_put(&is->videoq, &copy);
                packet_queue_put_nullpacket(&is->videoq, is->video_stream);
            }
            is->queue_attachments_req = 0;
        }

        /* if the queue are full, no need to read more */
        if (is->infinite_buffer<1 &&
            (is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE
             || (stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq) &&
                 stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq) &&
                 stream_has_enough_packets(is->subtitle_st, is->subtitle_stream, &is->subtitleq)))) {
            av_log(NULL, AV_LOG_DEBUG,"wait loop:%d, read:%d\n", is->loop, ret);
            /* wait 10 ms */
            gettimeofday(&now, NULL);
            timeout.tv_sec = now.tv_sec;
            timeout.tv_nsec = now.tv_usec * 1000 + 10 * 1000;
            pthread_mutex_lock(&wait_mutex);
            pthread_cond_timedwait(&is->continue_read_thread, &wait_mutex, &timeout);
            pthread_mutex_unlock(&wait_mutex);
            continue;
        }
        if (!is->paused &&
            (!is->audio_st || (is->auddec.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sampq) == 0)) &&
            (!is->video_st || (is->viddec.finished == is->videoq.serial && frame_queue_nb_remaining(&is->pictq) == 0))) {
            if (is->loop != 1 && (!is->loop || --is->loop)) {
                stream_seek(is, is->start_time != AV_NOPTS_VALUE ? is->start_time : 0, 0, 0);
            } else if (is->autoexit) {
                ret = AVERROR_EOF;
                goto fail;
            }
        }


        ret = av_read_frame(ic, pkt);
        av_log(NULL, AV_LOG_DEBUG,"loop:%d, read:%d\n", is->loop, ret);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
                if (is->video_stream >= 0)
                    packet_queue_put_nullpacket(&is->videoq, is->video_stream);
                if (is->audio_stream >= 0)
                    packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
                if (is->subtitle_stream >= 0)
                    packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);
                is->eof = 1;
            }
            if (ic->pb && ic->pb->error)
                break;

            gettimeofday(&now, NULL);
            timeout.tv_sec = now.tv_sec;
            timeout.tv_nsec = now.tv_usec * 1000 + 10 * 1000;
            pthread_mutex_lock(&wait_mutex);
            pthread_cond_timedwait(&is->continue_read_thread, &wait_mutex, &timeout);
            pthread_mutex_unlock(&wait_mutex);
            continue;
        } else {
            is->eof = 0;
        }

        /* check if packet is in play range specified by user, then queue, otherwise discard */
        stream_start_time = ic->streams[pkt->stream_index]->start_time;
        pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
        pkt_in_play_range = is->duration == AV_NOPTS_VALUE ||
                            (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
                            av_q2d(ic->streams[pkt->stream_index]->time_base) -
                            (double)(is->start_time != AV_NOPTS_VALUE ? is->start_time : 0) / 1000000
                            <= ((double)is->duration / 1000000);
        if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
            packet_queue_put(&is->audioq, pkt);
        } else if (pkt->stream_index == is->video_stream && pkt_in_play_range
                   && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            packet_queue_put(&is->videoq, pkt);
        } else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
            packet_queue_put(&is->subtitleq, pkt);
        } else {
            av_packet_unref(pkt);
        }
    }

    ret = 0;

    fail:
    if (ic && !is->ic)
        avformat_close_input(&ic);

    if (ret != 0) {
#if 0
        SDL_Event event;

        event.type = FF_QUIT_EVENT;
        event.user.data1 = is;
        SDL_PushEvent(&event);
#endif
    }
    pthread_mutex_destroy(&wait_mutex);

    av_log(NULL, AV_LOG_DEBUG, "read_thread leave\n");

    return 0;
}


static double compute_target_delay(double delay, VideoState *is)
{
    double sync_threshold, diff = 0;

    /* update delay to follow master synchronisation source */
    if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER) {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
        diff = get_clock(&is->vidclk) - get_master_clock(is);

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
            if (diff <= -sync_threshold)
                delay = FFMAX(0, delay + diff);
            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                delay = delay + diff;
            else if (diff >= sync_threshold)
                delay = 2 * delay;
        }
    }

    av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n",
           delay, -diff);

    return delay;
}

static double vp_duration(VideoState *is, Frame *vp, Frame *nextvp) {
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
            return vp->duration;
        else
            return duration;
    } else {
        return 0.0;
    }
}

static void update_video_pts(VideoState *is, double pts, int64_t pos, int serial) {
    /* update current video pts */
    set_clock(&is->vidclk, pts, serial);
    sync_clock_to_slave(&is->extclk, &is->vidclk);
}

static void get_android_pix_fmt(int format, enum AVPixelFormat *pix_fmt)
{
    int i;
    for (i = 0; i < FF_ARRAY_ELEMS(android_format_map) - 1; i++) {
        if (format == android_format_map[i].android_fmt) {
            *pix_fmt = android_format_map[i].format;
            return;
        }
    }
}


static int realloc_texture(VideoState *is, int new_format, int new_width, int new_height, enum AVPixelFormat pix_fmt)
{
    int format;
    int w, h;
    ANativeWindow *window = is->window;

    if (window && ((w = ANativeWindow_getWidth(window)) != new_width ||
            (h = ANativeWindow_getHeight(window)) != new_height ||
            (format = ANativeWindow_getFormat(window)) != new_format) ) {

        av_log(NULL, AV_LOG_VERBOSE, "Old %dx%d window with %d.\n", w, h, format);

        if (w != new_width || h != new_height)
            is->image_size = 0;

        ANativeWindow_setBuffersGeometry(window, new_width, new_height, new_format);

        av_log(NULL, AV_LOG_VERBOSE, "Created %dx%d window with %d.\n", new_width, new_height, ANativeWindow_getFormat(window));
    }

    if (is->image_size <= 0) {
        av_freep(&is->pixels[0]);

        /* buffer is going to be written to rawvideo file, no alignment */
        if ((is->image_size = av_image_alloc(is->pixels, is->pitch,
                                         new_width, new_height, pix_fmt, 1)) < 0) {
            av_log(NULL, AV_LOG_FATAL, "Could not allocate destination image %dx%d\n", new_width, new_height);
            return -1;
        }

        av_log(NULL, AV_LOG_VERBOSE, "Alloc image %dx%d out size %d.\n", new_width, new_height, is->image_size);
    }

    return 0;
}

#include <stdio.h>
static int upload_texture(VideoState *is, AVFrame *frame, struct SwsContext **img_convert_ctx) {

    enum AVPixelFormat pix_fmt;
    get_android_pix_fmt(is->window_format, &pix_fmt);

    if (realloc_texture(is, is->window_format, frame->width, frame->height, pix_fmt) < 0)
        return -1;

    *img_convert_ctx = sws_getCachedContext(*img_convert_ctx,
                                            frame->width, frame->height,
                                            static_cast<AVPixelFormat>(frame->format), frame->width, frame->height,
                                            pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

    sws_scale(is->img_convert_ctx, (const uint8_t * const *)frame->data, frame->linesize,
              0, frame->height, is->pixels, is->pitch);

    av_log(NULL, AV_LOG_DEBUG, "scale %d*%d %x*%d %x*%d %x*%d\n"
           , frame->width, frame->height
           , is->pixels[0], is->pitch[0]
           , is->pixels[1], is->pitch[1]
           , is->pixels[2], is->pitch[2]);

#if 0
    FILE *fp = fopen("/data/data/com.kotlin.media/files/a.png", "wb");
    if (fp)
    {
        fwrite(is->pixels[0], is->image_size, 1, fp);
        fclose(fp);
    }
    else
        av_log(NULL, AV_LOG_FATAL, "open file failed %s\n", "/data/data/com.kotlin.media/files/a.png");
#endif

    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(is->window, &window_buffer, 0))
    {
        av_log(NULL, AV_LOG_FATAL, "lock window failed %d\n", is->window);
        goto end;
    }

    memcpy(window_buffer.bits, is->pixels[0], is->image_size);

    ANativeWindow_unlockAndPost(is->window);

    end:
    return 0;
}

static void video_image_display(VideoState *is)
{
    Frame *vp;
    Frame *sp = NULL;

    vp = frame_queue_peek_last(&is->pictq);
#if 1

    if (!vp->uploaded) {
        if (upload_texture(is, vp->frame, &is->img_convert_ctx) < 0)
            return;
        vp->uploaded = 1;
        vp->flip_v = vp->frame->linesize[0] < 0;
    }

#else
    if (false && is->surface)
    {
        AVMediaCodecBuffer *buffer = (AVMediaCodecBuffer *) vp->frame->data[3];
        if (buffer)
            av_mediacodec_release_buffer(buffer, 1);
    }

    if (is->subtitle_st) {
        if (frame_queue_nb_remaining(&is->subpq) > 0) {
            sp = frame_queue_peek(&is->subpq);

            if (vp->pts >= sp->pts + ((float) sp->sub.start_display_time / 1000)) {
                if (!sp->uploaded) {
                    uint8_t* pixels[4];
                    int pitch[4];
                    int i;
                    if (!sp->width || !sp->height) {
                        sp->width = vp->width;
                        sp->height = vp->height;
                    }
                    if (realloc_texture(&is->sub_texture, SDL_PIXELFORMAT_ARGB8888, sp->width, sp->height, SDL_BLENDMODE_BLEND, 1) < 0)
                        return;

                    for (i = 0; i < sp->sub.num_rects; i++) {
                        AVSubtitleRect *sub_rect = sp->sub.rects[i];

                        sub_rect->x = av_clip(sub_rect->x, 0, sp->width );
                        sub_rect->y = av_clip(sub_rect->y, 0, sp->height);
                        sub_rect->w = av_clip(sub_rect->w, 0, sp->width  - sub_rect->x);
                        sub_rect->h = av_clip(sub_rect->h, 0, sp->height - sub_rect->y);

                        is->sub_convert_ctx = sws_getCachedContext(is->sub_convert_ctx,
                                                                   sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
                                                                   sub_rect->w, sub_rect->h, AV_PIX_FMT_BGRA,
                                                                   0, NULL, NULL, NULL);
                        if (!is->sub_convert_ctx) {
                            av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
                            return;
                        }
                        if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)pixels, pitch)) {
                            sws_scale(is->sub_convert_ctx, (const uint8_t * const *)sub_rect->data, sub_rect->linesize,
                                      0, sub_rect->h, pixels, pitch);
                            SDL_UnlockTexture(is->sub_texture);
                        }
                    }
                    sp->uploaded = 1;
                }
            } else
                sp = NULL;
        }
    }


    calculate_display_rect(&rect, is->xleft, is->ytop, is->width, is->height, vp->width, vp->height, vp->sar);

    if (!vp->uploaded) {
        if (upload_texture(&is->vid_texture, vp->frame, &is->img_convert_ctx) < 0)
            return;
        vp->uploaded = 1;
        vp->flip_v = vp->frame->linesize[0] < 0;
    }

    SDL_RenderCopyEx(renderer, is->vid_texture, NULL, &rect, 0, NULL, vp->flip_v ? SDL_FLIP_VERTICAL : 0);
    if (sp) {
#if USE_ONEPASS_SUBTITLE_RENDER
       SDL_RenderCopy(renderer, is->sub_texture, NULL, &rect);
#else
        int i;
        double xratio = (double)rect.w / (double)sp->width;
        double yratio = (double)rect.h / (double)sp->height;
        for (i = 0; i < sp->sub.num_rects; i++) {
            SDL_Rect *sub_rect = (SDL_Rect*)sp->sub.rects[i];
            SDL_Rect target = {.x = rect.x + sub_rect->x * xratio,
                               .y = rect.y + sub_rect->y * yratio,
                               .w = sub_rect->w * xratio,
                               .h = sub_rect->h * yratio};
            SDL_RenderCopy(renderer, is->sub_texture, sub_rect, &target);
        }
#endif
    }
#endif
}

static inline int compute_mod(int a, int b)
{
    return a < 0 ? a%b + b : a%b;
}

static void video_audio_display(VideoState *s)
{
    int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
    int ch, channels, h, h2;
    int64_t time_diff;
    int rdft_bits, nb_freq;

    for (rdft_bits = 1; (1 << rdft_bits) < 2 * s->height; rdft_bits++)
        ;
    nb_freq = 1 << (rdft_bits - 1);

    /* compute display index : center on currently output samples */
    channels = s->audio_tgt.channels;
    nb_display_channels = channels;
    if (!s->paused) {
        int data_used= s->show_mode == VideoState::SHOW_MODE_WAVES ? s->width : (2*nb_freq);
        n = 2 * channels;
        delay = s->audio_write_buf_size;
        delay /= n;

        /* to be more precise, we take into account the time spent since
           the last buffer computation */
        if (s->audio_callback_time) {
            time_diff = av_gettime_relative() - s->audio_callback_time;
            delay -= (time_diff * s->audio_tgt.freq) / 1000000;
        }

        delay += 2 * data_used;
        if (delay < data_used)
            delay = data_used;

        i_start= x = compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
        if (s->show_mode == VideoState::SHOW_MODE_WAVES) {
            h = INT_MIN;
            for (i = 0; i < 1000; i += channels) {
                int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
                int a = s->sample_array[idx];
                int b = s->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
                int c = s->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
                int d = s->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
                int score = a - d;
                if (h < score && (b ^ c) < 0) {
                    h = score;
                    i_start = idx;
                }
            }
        }

        s->last_i_start = i_start;
    } else {
        i_start = s->last_i_start;
    }

#if 0
    if (s->show_mode == VideoState::SHOW_MODE_WAVES) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        /* total height for one channel */
        h = s->height / nb_display_channels;
        /* graph height / 2 */
        h2 = (h * 9) / 20;
        for (ch = 0; ch < nb_display_channels; ch++) {
            i = i_start + ch;
            y1 = s->ytop + ch * h + (h / 2); /* position of center line */
            for (x = 0; x < s->width; x++) {
                y = (s->sample_array[i] * h2) >> 15;
                if (y < 0) {
                    y = -y;
                    ys = y1 - y;
                } else {
                    ys = y1;
                }
                fill_rectangle(s->xleft + x, ys, 1, y);
                i += channels;
                if (i >= SAMPLE_ARRAY_SIZE)
                    i -= SAMPLE_ARRAY_SIZE;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

        for (ch = 1; ch < nb_display_channels; ch++) {
            y = s->ytop + ch * h;
            fill_rectangle(s->xleft, y, s->width, 1);
        }
    } else {
        if (realloc_texture(&s->vis_texture, SDL_PIXELFORMAT_ARGB8888, s->width, s->height, SDL_BLENDMODE_NONE, 1) < 0)
            return;

        nb_display_channels= FFMIN(nb_display_channels, 2);
        if (rdft_bits != s->rdft_bits) {
            av_rdft_end(s->rdft);
            av_free(s->rdft_data);
            s->rdft = av_rdft_init(rdft_bits, DFT_R2C);
            s->rdft_bits = rdft_bits;
            s->rdft_data = av_malloc_array(nb_freq, 4 *sizeof(*s->rdft_data));
        }
        if (!s->rdft || !s->rdft_data){
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate buffers for RDFT, switching to waves display\n");
            s->show_mode = SHOW_MODE_WAVES;
        } else {
            FFTSample *data[2];
            SDL_Rect rect = {.x = s->xpos, .y = 0, .w = 1, .h = s->height};
            uint32_t *pixels;
            int pitch;
            for (ch = 0; ch < nb_display_channels; ch++) {
                data[ch] = s->rdft_data + 2 * nb_freq * ch;
                i = i_start + ch;
                for (x = 0; x < 2 * nb_freq; x++) {
                    double w = (x-nb_freq) * (1.0 / nb_freq);
                    data[ch][x] = s->sample_array[i] * (1.0 - w * w);
                    i += channels;
                    if (i >= SAMPLE_ARRAY_SIZE)
                        i -= SAMPLE_ARRAY_SIZE;
                }
                av_rdft_calc(s->rdft, data[ch]);
            }
            /* Least efficient way to do this, we should of course
             * directly access it but it is more than fast enough. */
            if (!SDL_LockTexture(s->vis_texture, &rect, (void **)&pixels, &pitch)) {
                pitch >>= 2;
                pixels += pitch * s->height;
                for (y = 0; y < s->height; y++) {
                    double w = 1 / sqrt(nb_freq);
                    int a = sqrt(w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] + data[0][2 * y + 1] * data[0][2 * y + 1]));
                    int b = (nb_display_channels == 2 ) ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1]))
                                                        : a;
                    a = FFMIN(a, 255);
                    b = FFMIN(b, 255);
                    pixels -= pitch;
                    *pixels = (a << 16) + (b << 8) + ((a+b) >> 1);
                }
                SDL_UnlockTexture(s->vis_texture);
            }
            SDL_RenderCopy(renderer, s->vis_texture, NULL, NULL);
        }
        if (!s->paused)
            s->xpos++;
        if (s->xpos >= s->width)
            s->xpos= s->xleft;
    }
#endif
}

/* display the current picture, if any */
static void video_display(VideoState *is)
{
    //if (!is->width)
    //    video_open(is);

    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    //SDL_RenderClear(renderer);
    //if (is->audio_st && is->show_mode != VideoState::SHOW_MODE_VIDEO)
    //    video_audio_display(is);
    //else
    if (is->video_st)
        video_image_display(is);
    //SDL_RenderPresent(renderer);
}

/* called to display each frame */
static void video_refresh(VideoState *is, double *remaining_time)
{
    double time;

    Frame *sp, *sp2;

    if (!is->paused && get_master_sync_type(is) == AV_SYNC_EXTERNAL_CLOCK && is->realtime)
        check_external_clock_speed(is);

    if (!is->display_disable && is->show_mode != VideoState::SHOW_MODE_VIDEO && is->audio_st) {
        time = av_gettime_relative() / 1000000.0;
        if (is->force_refresh || is->last_vis_time + is->rdftspeed < time) {
            video_display(is);
            is->last_vis_time = time;
        }
        *remaining_time = FFMIN(*remaining_time, is->last_vis_time + is->rdftspeed - time);
    }

    if (is->video_st) {
        retry:
        if (frame_queue_nb_remaining(&is->pictq) == 0) {
            // nothing to do, no picture to display in the queue
        } else {
            double last_duration, duration, delay;
            Frame *vp, *lastvp;

            /* dequeue the picture */
            lastvp = frame_queue_peek_last(&is->pictq);
            vp = frame_queue_peek(&is->pictq);

            if (vp->serial != is->videoq.serial) {
                frame_queue_next(&is->pictq);
                goto retry;
            }

            if (lastvp->serial != vp->serial)
                is->frame_timer = av_gettime_relative() / 1000000.0;

            if (is->paused)
                goto display;

            /* compute nominal last_duration */
            last_duration = vp_duration(is, lastvp, vp);
            delay = compute_target_delay(last_duration, is);


            time= av_gettime_relative()/1000000.0;
            //av_log(NULL, AV_LOG_DEBUG, "wait tarrget delay %d %ld %d\n", delay, time, is->frame_timer);
            if (time < is->frame_timer + delay) {
                *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
                goto display;
            }

            is->frame_timer += delay;
            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
                is->frame_timer = time;

            pthread_mutex_lock(&is->pictq.mutex);
            if (!isnan(vp->pts))
                update_video_pts(is, vp->pts, vp->pos, vp->serial);
            pthread_mutex_unlock(&is->pictq.mutex);

            if (frame_queue_nb_remaining(&is->pictq) > 1) {
                Frame *nextvp = frame_queue_peek_next(&is->pictq);
                duration = vp_duration(is, vp, nextvp);
                if(!is->step && (is->framedrop>0 || (is->framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) && time > is->frame_timer + duration){
                    is->frame_drops_late++;
                    frame_queue_next(&is->pictq);
                    goto retry;
                }
            }

            if (is->subtitle_st) {
                while (frame_queue_nb_remaining(&is->subpq) > 0) {
                    sp = frame_queue_peek(&is->subpq);

                    if (frame_queue_nb_remaining(&is->subpq) > 1)
                        sp2 = frame_queue_peek_next(&is->subpq);
                    else
                        sp2 = NULL;

                    if (sp->serial != is->subtitleq.serial
                        || (is->vidclk.pts > (sp->pts + ((float) sp->sub.end_display_time / 1000)))
                        || (sp2 && is->vidclk.pts > (sp2->pts + ((float) sp2->sub.start_display_time / 1000))))
                    {
                        if (sp->uploaded) {
                            int i;
                            for (i = 0; i < sp->sub.num_rects; i++) {
                                AVSubtitleRect *sub_rect = sp->sub.rects[i];
                                uint8_t *pixels;
                                int pitch, j;
#if 0
                                if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)&pixels, &pitch)) {
                                    for (j = 0; j < sub_rect->h; j++, pixels += pitch)
                                        memset(pixels, 0, sub_rect->w << 2);
                                    SDL_UnlockTexture(is->sub_texture);
                                }
#endif
                            }
                        }
                        frame_queue_next(&is->subpq);
                    } else {
                        break;
                    }
                }
            }

            frame_queue_next(&is->pictq);
            is->force_refresh = 1;

            if (is->step && !is->paused)
                stream_toggle_pause(is);
        }
        display:
        /* display picture */
        if (!is->display_disable && is->force_refresh && is->show_mode == VideoState::SHOW_MODE_VIDEO && is->pictq.rindex_shown)
            video_display(is);
    }
    is->force_refresh = 0;
    if (is->show_status) {
        static int64_t last_time;
        int64_t cur_time;
        int aqsize, vqsize, sqsize;
        double av_diff;

        cur_time = av_gettime_relative();
        if (!last_time || (cur_time - last_time) >= 30000) {
            aqsize = 0;
            vqsize = 0;
            sqsize = 0;
            if (is->audio_st)
                aqsize = is->audioq.size;
            if (is->video_st)
                vqsize = is->videoq.size;
            if (is->subtitle_st)
                sqsize = is->subtitleq.size;
            av_diff = 0;
            if (is->audio_st && is->video_st)
                av_diff = get_clock(&is->audclk) - get_clock(&is->vidclk);
            else if (is->video_st)
                av_diff = get_master_clock(is) - get_clock(&is->vidclk);
            else if (is->audio_st)
                av_diff = get_master_clock(is) - get_clock(&is->audclk);
            av_log(NULL, AV_LOG_INFO,
                   "%7.2f %s:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB f=%" PRId64"/%" PRId64"   \r",
                   get_master_clock(is),
                   (is->audio_st && is->video_st) ? "A-V" : (is->video_st ? "M-V" : (is->audio_st ? "M-A" : "   ")),
                   av_diff,
                   is->frame_drops_early + is->frame_drops_late,
                   aqsize / 1024,
                   vqsize / 1024,
                   sqsize,
                   is->video_st ? is->viddec.avctx->pts_correction_num_faulty_dts : 0,
                   is->video_st ? is->viddec.avctx->pts_correction_num_faulty_pts : 0);
            fflush(stdout);
            last_time = cur_time;
        }
    }
}

static int refresh_loop_wait_event(VideoState *is, MyEvent *event) {
    int ret;
    double remaining_time = 0.0;
    while ((ret = event_queue_get(is->looper.queue, event, 0)) == 0) {
        //av_log(NULL, AV_LOG_DEBUG, "wait video refresh %d\n", remaining_time);
        if (remaining_time > 0.0)
            av_usleep((int64_t)(remaining_time * 1000000.0));
        remaining_time = REFRESH_RATE;
        if (is->show_mode != VideoState::SHOW_MODE_NONE && (!is->paused || is->force_refresh))
            video_refresh(is, &remaining_time);
    }

    return ret;
}


static void *event_thread(void *arg) {
    VideoState *is = (VideoState *) arg;
    int ret;
    MyEvent event;
    double incr, pos, frac;

    av_log(NULL, AV_LOG_DEBUG, "event_thread enter\n");

    for ( ; ; ) {
        //获取事件
        ret = refresh_loop_wait_event(is, &event);
        if (ret < 0)
            break;

        // 处理event
        av_log(NULL, AV_LOG_DEBUG, "event_thread code: %d\n", event.code);
        switch (event.code) {
            case FF_EVENT_TOOGLE_PAUSE:
                toggle_pause(is);
                av_log(NULL, AV_LOG_DEBUG, "event_thread is paused: %d\n", is->paused);
                break;
            case FF_EVENT_TOOGLE_MUTE:
                toggle_mute(is);
                break;
            case FF_EVENT_INC_VOLUME:
                update_volume(is, 1, SDL_VOLUME_STEP);
                break;
            case FF_EVENT_DEC_VOLUME:
                update_volume(is, -1, SDL_VOLUME_STEP);
                break;
            case FF_EVENT_NEXT_FRAME:
                step_to_next_frame(is);
                break;
            case FF_EVENT_FAST_BACK:
                incr = is->seek_interval ? -is->seek_interval : -10.0;
                goto do_seek;
            case FF_EVENT_FAST_FORWARD:
                incr = is->seek_interval ? is->seek_interval : 10.0;
            do_seek:
                    if (is->seek_by_bytes) {
                        pos = -1;
                        if (pos < 0 && is->video_stream >= 0)
                            pos = frame_queue_last_pos(&is->pictq);
                        if (pos < 0 && is->audio_stream >= 0)
                            pos = frame_queue_last_pos(&is->sampq);
                        if (pos < 0)
                            pos = avio_tell(is->ic->pb);
                        if (is->ic->bit_rate)
                            incr *= is->ic->bit_rate / 8.0;
                        else
                            incr *= 180000.0;
                        pos += incr;
                        stream_seek(is, pos, incr, 1);
                    } else {
                        pos = get_master_clock(is);
                        if (isnan(pos))
                            pos = (double)is->seek_pos / AV_TIME_BASE;
                        pos += incr;
                        if (is->ic->start_time != AV_NOPTS_VALUE && pos < is->ic->start_time / (double)AV_TIME_BASE)
                            pos = is->ic->start_time / (double)AV_TIME_BASE;
                        stream_seek(is, (int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE), 0);
                    }
                break;
            default:
                break;
        }

        if (event.data)
            av_free(event.data);
    }

    av_log(NULL, AV_LOG_DEBUG, "event_thread leave\n");

    return 0;
}

VideoState *stream_open(const char *filename, AVInputFormat *iformat, void *surface)
{
    VideoState *is;
    int ret;

    av_log(NULL, AV_LOG_DEBUG, "stream_open %s\n", filename);

    is = (VideoState *)av_mallocz(sizeof(VideoState));
    if (!is)
        return NULL;
    is->last_video_stream = is->video_stream = -1;
    is->last_audio_stream = is->audio_stream = -1;
    is->last_subtitle_stream = is->subtitle_stream = -1;
    is->filename = av_strdup(filename);
    if (!is->filename)
        goto fail;
    is->iformat = iformat;
    is->surface = surface;
    is->window  = 0;
    is->ytop    = 0;
    is->xleft   = 0;

    is->fast    = 0;
    is->genpts  = 0;
    is->lowres  = 0;

    is->framedrop               = -1;
    is->decoder_reorder_pts     = -1;
    is->show_status             = 1;
    is->loop                    = 1;
    is->autoexit                = 1;

    is->seek_by_bytes           = -1;
    is->seek_interval           = 10;

    is->start_time              = AV_NOPTS_VALUE;
    is->duration                = AV_NOPTS_VALUE;

    is->display_disable     = 0;
    is->audio_disable       = 0;
    is->video_disable       = 0;
    is->subtitle_disable    = 0;

    is->infinite_buffer     = -1;

    is->rdftspeed               = 0.02;
    is->audio_callback_time     = 0;

    is->audios                  = 0;

    //android
    is->window_format           = WINDOW_FORMAT_RGBA_8888;

    is->image_size              = 0;

    memset(is->wanted_stream_spec, 0, sizeof(is->wanted_stream_spec));

    av_init_packet(&is->flush_pkt);
    is->flush_pkt.data = (uint8_t *)&is->flush_pkt;

    /* start video display */
    if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
        goto fail;
    if (frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0)
        goto fail;
    if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0)
        goto fail;

    is->pictq.is = is;
    is->subpq.is = is;
    is->sampq.is = is;

    if (packet_queue_init(&is->videoq) < 0 ||
        packet_queue_init(&is->audioq) < 0 ||
        packet_queue_init(&is->subtitleq) < 0)
        goto fail;

    is->videoq.is = is;
    is->audioq.is = is;
    is->subtitleq.is = is;

    if (event_queue_init(&is->eventq) < 0)
        goto fail;

    is->eventq.is = is;

    if ((ret = pthread_cond_init(&is->continue_read_thread, NULL)) != 0) {
        av_log(NULL, AV_LOG_FATAL, "pthread_cond_init(): %s\n", strerror(ret));
        goto fail;
    }

    init_clock(&is->vidclk, &is->videoq.serial);
    init_clock(&is->audclk, &is->audioq.serial);
    init_clock(&is->extclk, &is->extclk.serial);
    is->audio_clock_serial = -1;
#if 0
    if (startup_volume < 0)
        av_log(NULL, AV_LOG_WARNING, "-volume=%d < 0, setting to 0\n", startup_volume);
    if (startup_volume > 100)
        av_log(NULL, AV_LOG_WARNING, "-volume=%d > 100, setting to 100\n", startup_volume);
    startup_volume = av_clip(startup_volume, 0, 100);
    startup_volume = av_clip(SDL_MIX_MAXVOLUME * startup_volume / 100, 0, SDL_MIX_MAXVOLUME);
    is->audio_volume = startup_volume;
    is->av_sync_type = av_sync_type;
#else
    is->audio_volume = 100;
    is->av_sync_type = AV_SYNC_AUDIO_MASTER;
#endif
    is->muted = 0;

    if ((ret = pthread_create(&is->read_tid, NULL, read_thread, is)) != 0) {
        av_log(NULL, AV_LOG_FATAL, "pthread_create(): %s\n", strerror(ret));
        goto fail;
    }

    looper_init(&is->looper, &is->eventq);

    if ((ret = looper_start(&is->looper, event_thread, "event_looper", is)) < 0)
        goto fail;

    av_log(NULL, AV_LOG_DEBUG, "stream_open Success %s\n", filename);
    return is;

fail:
    stream_close(is);
    return NULL;
}

void stream_close(VideoState *is)
{
    av_log(NULL, AV_LOG_DEBUG, "stream_close\n");

    is->abort_request = 1;

    pthread_cond_signal(&is->continue_read_thread);

    pthread_join(is->read_tid, 0);

    /* close each stream */
    if (is->audio_stream >= 0)
        stream_component_close(is, is->audio_stream);
    if (is->video_stream >= 0)
        stream_component_close(is, is->video_stream);
    if (is->subtitle_stream >= 0)
        stream_component_close(is, is->subtitle_stream);

    looper_abort(&is->looper);
    looper_destroy(&is->looper);

    avformat_close_input(&is->ic);

    packet_queue_destroy(&is->videoq);
    packet_queue_destroy(&is->audioq);
    packet_queue_destroy(&is->subtitleq);

    /* free all pictures */
    frame_queue_destory(&is->pictq);
    frame_queue_destory(&is->sampq);
    frame_queue_destory(&is->subpq);

    event_queue_destroy(&is->eventq);

    pthread_cond_destroy(&is->continue_read_thread);
    sws_freeContext(is->img_convert_ctx);
    //sws_freeContext(is->sub_convert_ctx);
    av_free(is->filename);
    av_freep(&is->pixels[0]);

#if 0
    if (is->vis_texture)
        SDL_DestroyTexture(is->vis_texture);
    if (is->vid_texture)
        SDL_DestroyTexture(is->vid_texture);
    if (is->sub_texture)
        SDL_DestroyTexture(is->sub_texture);
#endif
    av_free(is);
}

void send_event(VideoState *is, enum ffEvent e)
{
    MyEvent evt;
    evt.code = e;
    evt.data = 0;
    evt.size = 0;
    event_queue_put(&is->eventq, &evt);
}


void set_seek_by_bytes(VideoState *is, int seek_by_bytes)
{
    is->seek_by_bytes = seek_by_bytes;
}

void set_seek_interval(VideoState *is, int seek_interval)
{
    is->seek_interval = seek_interval;
}

bool is_paused(VideoState *is)
{
    return is->paused;
}

bool is_mute(VideoState *is)
{
    return is->muted;
}

const char *stream_filename(VideoState *is)
{
    return is->filename;
}

void *stream_surface(VideoState *is)
{
    return is->surface;
}