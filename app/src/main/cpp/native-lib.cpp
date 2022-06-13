#include <jni.h>
#include <media/NdkImage.h>
#include <android/native_window_jni.h>
#include <string>
#include <stdint.h>
#include <unistd.h>
#include "libyuv.h"
#include "AMediaDecoder.h"
#include "TCPSocket.h"
#include "threadsafe_queue.h"
#include "QueueWindowData.h"
#include "time_util.h"
#include "ffplay.h"

extern "C"
{
#include "libavcodec/jni.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/mediacodec.h"
#include "libavutil/hwcontext_mediacodec.h"
};

#include <android/log.h>
#if 0
#define LOGV(...)
#define LOGE(...)
#else
#define TAG "native-lib"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#endif


QueueWindowData *g_data = new QueueWindowData();


extern "C" JNIEXPORT jstring JNICALL
Java_com_kotlin_media_DeviceSurface_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {

    std::string hello = "";

    {
        threadsafe_queue<std::string> queue;
        queue.push("Hello");;
        queue.push("from");
        queue.push("C++ ");
        queue.push("!");

        while (!queue.empty()) {
            hello += queue.wait_and_pop();
            hello += " ";
        }
    }
    hello += "\r\n";

    {
        AMediaDecoder decoder("video/avc");
        int code = decoder.open(100, 100, 19);
        hello += "open decoder:";
        hello += std::to_string(code);

        if (0 == code)
        {
            hello += ", fmt=";
            hello += std::to_string(decoder.fmt());

            decoder.close();
        }
    }
    hello += "\r\n";


#if 0
    {
        TCPSocket socket;
        int code = socket.open("192.168.137.1", 60000);
        hello += "open socket:";
        hello += std::to_string(code);

        if (0 == code) {
            const char *data = "hello socket!";
            ssize_t len = socket.send(data, strlen(data));

            hello += "  send socket:";
            hello += std::to_string(len);

            char buff[1024];
            memset(buff, sizeof(buff), 0);

            len = socket.recv(buff, sizeof(buff));
            hello += "  recv socket:";
            hello += std::to_string(len);

            if (len > 0) {
                hello += "\r\n";
                hello += std::string(buff, len);
            }

            socket.close();
        }
    }
    hello += "\r\n";
#endif


    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_kotlin_media_DeviceSurface_init(
        JNIEnv* env,
        jobject /* this */,
        jstring ip,
        jint port,
        jint test) {

    if (NULL == g_data)
        return;

    const char *utf8 = env->GetStringUTFChars(ip, NULL);

    g_data->init(utf8, port, test);

    env->ReleaseStringUTFChars(ip, utf8);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kotlin_media_DeviceSurface_close(
        JNIEnv* env,
        jobject /* this */) {
    if (NULL == g_data)
        return;

    g_data->close();
}

jobject g_surface = NULL;

extern "C" JNIEXPORT jlong JNICALL
Java_com_kotlin_media_DeviceSurface_append(
        JNIEnv* env,
        jobject /* this */,
        jint id,
        jobject surface) {

    g_surface = surface;

    LOGV("init set surface %x", g_surface);

    //g_surface = (*env)->NewGlobalRef(env, surface);

    g_surface = env->NewGlobalRef(surface);

    LOGV("init set surface %x", g_surface);

    if (NULL == g_data)
        return 0;

    return g_data->append(env, id, surface);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kotlin_media_DeviceSurface_remove(
        JNIEnv* env,
        jobject /* this */,
        jint id,
        jlong window) {

    if (NULL == g_surface)
        return;

    env->DeleteGlobalRef(g_surface);
    g_surface = NULL;

    if (NULL == g_data)
        return;

    g_data->remove(id, window);
}


void android_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    va_list vl2;
    char *line = (char*)malloc(1024 * sizeof(char));
    static int print_prefix = 1;
    va_copy(vl2, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, 1024, &print_prefix);
    va_end(vl2);
    line[1023] = '\0';
    if (level > AV_LOG_WARNING) {
        LOGV("%s", line);
    } else{
        LOGE("%s", line);
    }
    free(line);
}

typedef struct MediaContext {
    char url[256];
    bool read_exit;
    pthread_t read_thread;
} MediaContext;

static MediaContext *g_media_ctx;

extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *res) {

    av_log_set_level(AV_LOG_TRACE);

    av_log_set_callback(android_log_callback);

    av_jni_set_java_vm(vm, 0);

    //avdevice_register_all();

    /* register all formats and codecs */
    //av_register_all();
    g_media_ctx = (MediaContext *)malloc(sizeof(MediaContext));


    return JNI_VERSION_1_6;
}



static int video_frame_count = 0;
static int audio_frame_count = 0;
static enum AVPixelFormat hw_pix_fmt;

static int output_video_frame(AVFrame *frame)
{
    int ret;
    AVFrame *sw_frame = NULL;

    LOGV("video_frame n:%d coded_n:%d, format:%d, width:%d, height: %d\n",
           video_frame_count++,
           frame->coded_picture_number,
           frame->format,
           frame->width,
           frame->height);

#if 0
    if ( frame->format == hw_pix_fmt )
    {
        if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
            fprintf(stderr, "Error transferring the data to system memory\n");
            return -1;
        }
        frame = sw_frame;
    }
#else
    AVMediaCodecBuffer *buffer = (AVMediaCodecBuffer *) frame->data[3];
    av_mediacodec_release_buffer(buffer, 1);
#endif

    return 0;
}

static int output_audio_frame(AVFrame *frame)
{
    LOGV("audio_frame n:%d, format:%d, width:%d, height: %d\n",
         audio_frame_count++,
         frame->format,
         frame->width,
         frame->height);

    return 0;
}

static int decode_packet(AVCodecContext *dec, const AVPacket *pkt, AVFrame *frame)
{
    int ret = 0;

    // submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0) {
        LOGE("Error submitting a packet for decoding (%s)\n", av_err2str(ret));
        return ret;
    }

    // get all the available frames from the decoder
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec, frame);
        if (ret < 0) {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return 0;

            LOGE("Error during decoding (%s)\n", av_err2str(ret));
            return ret;
        }

        // write the frame data to output file
        if (dec->codec->type == AVMEDIA_TYPE_VIDEO)
            ret = output_video_frame(frame);
        else
            ret = output_audio_frame(frame);

        av_frame_unref(frame);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
    int err = 0;
    AVBufferRef *hw_device_ctx = NULL;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        LOGE("Failed to create specified HW device.\n");
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


void *read_thread(void *arg)
{
    int ret, video_stream_idx;
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *video_dec_ctx = NULL;
    const AVCodec *video_dec = NULL;
    AVStream *video_st;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;
    AVMediaCodecContext *media_codec_ctx = NULL;
    int i;

    LOGV("enter read thread: ");

    MediaContext *media_ctx = (MediaContext *)arg;

    do {
        /* open input file, and allocate utf8 context */
        if (avformat_open_input(&fmt_ctx, media_ctx->url, NULL, NULL) < 0) {
            LOGE("Could not open source file %s\n", media_ctx->url);
            break;
        }
        LOGV("Success open source file %s\n", media_ctx->url);

        if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
            LOGE("Could not find stream information\n");
            break;
        }
        LOGV("Success find stream information\n");

        ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (ret < 0) {
            LOGE("Could not find %s stream in input file '%s'\n",
                 av_get_media_type_string(AVMEDIA_TYPE_VIDEO), media_ctx->url);
            break;
        }
        LOGV("Success find stream type:%s, nb_streams:%d, video_codec:%d\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO),
             fmt_ctx->nb_streams,
             fmt_ctx->video_codec);

        video_stream_idx = ret;
        video_st = fmt_ctx->streams[video_stream_idx];

#if 0
        if (st->parser && st->parser->parser)
            LOGV("parser ->  %d, %d, %d, %d",
                    st->parser->parser->codec_ids[0],
                    st->parser->parser->codec_ids[1],
                    AV_CODEC_ID_H264, AV_CODEC_ID_INDEO3);
#endif


        /* find decoder for the stream */
#if 1
        video_dec = avcodec_find_decoder(video_st->codecpar->codec_id);
        if (!video_dec) {
            LOGE("Failed to find %s codec\n",
                    av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            break;
        }
        LOGV("Success to find %s codec, name:%s\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO), video_dec->name);
#else
        video_dec = avcodec_find_decoder_by_name("h264_mediacodec");
        if (!video_dec) {
            LOGE("Failed to find %s codec\n",
                 av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            break;
        }
        LOGV("Success to find %s codec, name:%s\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO), video_dec->name);

#endif


        if (!video_st->codecpar->extradata) {
            //unsigned char sps_pps[23] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x0f, 0x00, 0x44, 0xbe, 0x8,
            //                              0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };

            unsigned char data[23] = {
                    0x00, 0x00, 0x00, 0x01, 0x67, 0x4D, 0x00, 0x2A, 0x95, 0xA8, 0x1E, 0x00, 0x89,
                    0xF9, 0x50, 0x00,
                    0x00, 0x00, 0x01, 0x68, 0xEE, 0x3C, 0x80
            };

            video_st->codecpar->extradata_size = 23;
            video_st->codecpar->extradata = (uint8_t *) av_malloc(
                    video_st->codecpar->extradata_size);
            if (video_st->codecpar->extradata) {
                memcpy(video_st->codecpar->extradata, data, video_st->codecpar->extradata_size);
                LOGV("av_malloc the video params extradata!\n");
            } else {
                LOGE("could not av_malloc the video params extradata!\n");
            }
        }

        enum AVHWDeviceType hw_type;
        hw_type = av_hwdevice_find_type_by_name("mediacodec");
        if (hw_type == AV_HWDEVICE_TYPE_NONE) {
            LOGE("Device type %s is not supported.\n", "mediacodec");
            LOGE("Available device types:");
            while ((hw_type = av_hwdevice_iterate_types(hw_type)) != AV_HWDEVICE_TYPE_NONE)
                LOGV(" %s", av_hwdevice_get_type_name(hw_type));
            break;
        }

        for (i = 0;; i++) {
            const AVCodecHWConfig *config = avcodec_get_hw_config(video_dec, i);
            if (!config) {
                LOGE("Decoder %s does not support device type %s.\n",
                        video_dec->name, av_hwdevice_get_type_name(hw_type));
                break;
            }
            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                config->device_type == hw_type) {
                hw_pix_fmt = config->pix_fmt;
                break;
            }
        }


#if 1
        /* Allocate a codec context for the decoder */
        video_dec_ctx = avcodec_alloc_context3(video_dec);
        if (!video_dec_ctx) {
            LOGE("Failed to allocate the %s codec context\n",
                 av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            break;
        }
        LOGV("Success to allocate the %s codec context\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO));

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(video_dec_ctx, video_st->codecpar)) < 0) {
            LOGE("Failed to copy %s codec parameters to decoder context\n",
                 av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            break;
        }
        LOGV("Success to copy %s codec parameters to decoder context, codec_id:%d:%s, pix_fmt:%d, width:%d, height:%d, extradata:%d, extradata_size:%d\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO),
             video_dec_ctx->codec_id,
             avcodec_get_name(video_dec_ctx->codec_id),
             video_dec_ctx->pix_fmt,
             video_dec_ctx->width,
             video_dec_ctx->height,
             video_dec_ctx->extradata,
             video_dec_ctx->extradata_size);

        if (video_dec_ctx->width == 0 && video_dec_ctx->height == 0) {
            video_dec_ctx->width = 1920;
            video_dec_ctx->height = 1080;
        }

        if (g_surface) {
            media_codec_ctx = av_mediacodec_alloc_context();
            if (!media_codec_ctx) {
                LOGE("Could not allocate mediacodec context\n");
                break;
            }

            LOGV("init mediacodec, surface:%x,  ret:%d\n",
                 g_surface, ret);


            if ((ret = av_mediacodec_default_init(video_dec_ctx, media_codec_ctx, g_surface)) < 0) {
                LOGE("Failed to init mediacodec, surface:%x,  ret:%d\n",
                     g_surface, ret);
                break;
            }

            LOGV("av_mediacodec_default_init, surface:%x,  ret:%d\n",
                 g_surface, ret);
        }


        if ((ret = hw_decoder_init(video_dec_ctx, hw_type)) < 0){
            LOGE("Failed hw_decoder_init,  ret:%d\n",
                 ret);
            break;
         }

        /* Init the decoders, with or without reference counting */
        if ((ret = avcodec_open2(video_dec_ctx, 0, 0)) < 0) {
            LOGE("Failed to open %s codec, ret:%d\n",
                 av_get_media_type_string(AVMEDIA_TYPE_VIDEO), ret);
            break;
        }
        LOGV("Success to open %s codec,  pix_fmt:%d\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO),
             video_dec_ctx->pix_fmt);

        frame = av_frame_alloc();
        if (!frame) {
            LOGE("Could not allocate frame\n");
            break;
        }

        pkt = av_packet_alloc();
        if (!pkt) {
            LOGE("Could not allocate packet\n");
            break;
        }

        /* read frames from the file */
        while (!media_ctx->read_exit && av_read_frame(fmt_ctx, pkt) >= 0) {
            // check if the packet belongs to a stream we are interested in, otherwise
            // skip it
            if (pkt->pts == AV_NOPTS_VALUE)
                pkt->pts = gettime_relative();

            if (pkt->stream_index == video_stream_idx)
                ret = decode_packet(video_dec_ctx, pkt, frame);

            av_packet_unref(pkt);
            if (ret < 0)
                break;

            if (media_ctx->read_exit)
                break;

            usleep(20000);
            //usleep(33000);
        }

        if (media_ctx->read_exit)
            break;

        if (video_dec_ctx)
            decode_packet(video_dec_ctx, NULL, frame);

#endif
    } while(0);

    if (fmt_ctx)
        avformat_close_input(&fmt_ctx);

    if (video_dec_ctx) {
        av_mediacodec_default_free(video_dec_ctx);
        avcodec_close(video_dec_ctx);
        avcodec_free_context(&video_dec_ctx);
    }

    av_packet_free(&pkt);
    av_frame_free(&frame);

    media_ctx->read_thread = 0;

    LOGV("leve read thread: ");
    return NULL;
}


int open_media(MediaContext *read_ctx)
{
    int ret;

    read_ctx->read_exit = false;
    read_ctx->read_thread = 0;

    if ((ret = pthread_create(&g_media_ctx->read_thread, NULL, read_thread, g_media_ctx)) != 0) {
        LOGE("can't create rad thread: %s\n", strerror(ret));
    }

    return ret;
}

int close_media(MediaContext *media_ctx)
{
    int ret = 0;

    media_ctx->read_exit = true;
    if (0 != media_ctx->read_thread) {
        ret = pthread_join(media_ctx->read_thread, 0);
        LOGV("stop read thread: %d\n", media_ctx->read_thread);
        media_ctx->read_thread = 0;
    }

    return ret;
}

VideoState *is = NULL;

extern "C" JNIEXPORT void JNICALL
Java_com_kotlin_media_DeviceSurface_openFfmpeg(
        JNIEnv* env,
        jobject /* this */,
        jstring url) {

    const char *utf8 = env->GetStringUTFChars(url, NULL);

    LOGV("open source file %s\n", utf8);

    int len = strlen(utf8);
    memcpy(g_media_ctx->url, utf8, len);
    g_media_ctx->url[len] = '\0';

#if 1
    //close_media(g_media_ctx);
    //open_media(g_media_ctx);
    is = stream_open(utf8, NULL, g_surface);

#else
    //read_thread(ctx);
#endif

    env->ReleaseStringUTFChars(url, utf8);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kotlin_media_DeviceSurface_closeFfmpeg(
        JNIEnv* env,
        jobject /* this */) {

    //close_media(g_media_ctx);
    if (is)
        stream_close(is);

    is = NULL;
}


extern "C" JNIEXPORT jlong JNICALL
Java_com_kotlin_media_DeviceSurface_ffmpegOpen(
        JNIEnv* env,
        jobject /* this */,
        jstring url,
        jobject surface) {

    const char *utf8 = env->GetStringUTFChars(url, NULL);
    jobject gsurface = env->NewGlobalRef(surface);

    LOGV("open source file %s, surface %x\n", utf8, gsurface);

    VideoState *is = stream_open(utf8, NULL, gsurface);

    env->ReleaseStringUTFChars(url, utf8);

    return (jlong)is;
}
extern "C" JNIEXPORT void JNICALL
Java_com_kotlin_media_DeviceSurface_ffmpegClose(
        JNIEnv* env,
        jobject /* this */,
        jlong handle) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);
    jobject gsurface = (jobject)stream_surface(is);

    LOGV("close source file %s, surface %x\n", utf8, gsurface);

    stream_close(is);

    env->DeleteGlobalRef(gsurface);
}
extern "C" JNIEXPORT void JNICALL
Java_com_kotlin_media_DeviceSurface_ffmpegSendEvent(
        JNIEnv* env,
        jobject /* this */,
        jlong handle,
        jlong code) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);

    LOGV("send event file: %s, code: %d\n", utf8, code);

    send_event(is, static_cast<ffEvent>(code));
}

extern "C" JNIEXPORT void JNICALL
Java_com_kotlin_media_DeviceSurface_ffmpegSetVolume(
        JNIEnv* env,
        jobject /* this */,
        jlong handle,
        jlong volume) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);

    LOGV("Java_com_kotlin_media_DeviceSurface_ffmpegSetVolume: %s, volume: %d\n", utf8, volume);

    //send_event(is, static_cast<ffEvent>(code));
    set_volume(is, volume);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_kotlin_media_DeviceSurface_ffmpegDuration(
        JNIEnv* env,
        jobject /* this */,
        jlong handle) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);

    int64_t duration = get_duration(is);

    LOGV("Java_com_kotlin_media_DeviceSurface_ffmpegDuration: %s, duration: %d\n", utf8, duration);

    return duration;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_kotlin_media_DeviceSurface_ffmpegCurrentDuration(
        JNIEnv* env,
        jobject /* this */,
        jlong handle) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);

    int64_t duration = get_current_duration(is);

    LOGV("Java_com_kotlin_media_DeviceSurface_ffmpegCurrentDuration: %s, duration: %d\n", utf8, duration);

    return duration;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kotlin_media_DeviceSurface_ffmpegSeek(
        JNIEnv* env,
        jobject /* this */,
        jlong handle,
        jlong pos) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);

    LOGV("Java_com_kotlin_media_DeviceSurface_ffmpegSeek: %s, pos: %d\n", utf8, pos);

    seek_pos(is, pos);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kotlin_media_DeviceSurface_ffmpegSetRate(
        JNIEnv* env,
        jobject /* this */,
        jlong handle,
        jint rate) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);

    LOGV("Java_com_kotlin_media_DeviceSurface_ffmpegSetRate: %s, rate: %d\n", utf8, rate);

    set_rate(is, rate);
}


void print_ffmpeg_info()
{
#if 0
    AVCodec *dec2 = av_codec_next(NULL);
    while (dec2)
    {
        LOGV("AVCodec codec：%d, name:%s, type:%s, is_decoder:%d, is_encoder:%d\n",
             dec2->id, dec2->name, av_get_media_type_string(dec2->type), av_codec_is_decoder(dec2), av_codec_is_encoder(dec2));

        dec2 = av_codec_next(dec2);
    }

    AVHWAccel *dec3 = av_hwaccel_next(NULL);
    while ( dec3 )
    {
        LOGV("AVHWAccel codec:%d, name:%s, type:%s\n",
             dec3->id, dec3->name, av_get_media_type_string(dec3->type));
        dec3 = av_hwaccel_next(dec3);
    }

    AVInputFormat *iforamt = av_iformat_next(NULL);
    while ( iforamt )
    {
        LOGV("AVInputFormat name:%s\n",
             iforamt->name);
        iforamt = av_iformat_next(iforamt);
    }

    AVOutputFormat *oforamt = av_oformat_next(NULL);
    while ( oforamt )
    {
        LOGV("AVOutputFormat name:%s\n",
             oforamt->name);
        oforamt = av_oformat_next(oforamt);
    }

    AVCodecParser *parser = av_parser_next(NULL);
    while ( parser )
    {
        LOGV("AVCodecParser %d:%s， %d:%s， %d:%s， %d:%s， %d:%s\n",
             parser->codec_ids[0], parser->codec_ids[0] ? avcodec_get_name((enum AVCodecID)parser->codec_ids[0]) : "",
             parser->codec_ids[1], parser->codec_ids[1] ? avcodec_get_name((enum AVCodecID)parser->codec_ids[1]) : "",
             parser->codec_ids[2], parser->codec_ids[2] ? avcodec_get_name((enum AVCodecID)parser->codec_ids[2]) : "",
             parser->codec_ids[3], parser->codec_ids[3] ? avcodec_get_name((enum AVCodecID)parser->codec_ids[3]) : "",
             parser->codec_ids[4], parser->codec_ids[4] ? avcodec_get_name((enum AVCodecID)parser->codec_ids[4]) : ""
             );

        parser = av_parser_next(parser);
    }


#endif
}
