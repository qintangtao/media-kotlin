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


extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *res) {

    av_log_set_level(AV_LOG_TRACE);

    av_log_set_callback(android_log_callback);

    av_jni_set_java_vm(vm, 0);

    //avdevice_register_all();

    /* register all formats and codecs */
    //av_register_all();

    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jlong JNICALL
Java_me_tang_ffplay_FFplay_open(
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
Java_me_tang_ffplay_FFplay_close(
        JNIEnv* env,
        jobject /* this */,
        jlong handle) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);
    jobject gsurface = (jobject)stream_surface(is);

    LOGV("close source file %s, surface %x\n", utf8, gsurface);

    stream_close(is);

    if (gsurface)
        env->DeleteGlobalRef(gsurface);
}

extern "C" JNIEXPORT void JNICALL
Java_me_tang_ffplay_FFplay_sendEvent(
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
Java_me_tang_ffplay_FFplay_setVolume(
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
Java_me_tang_ffplay_FFplay_duration(
        JNIEnv* env,
        jobject /* this */,
        jlong handle) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);

    int64_t duration = get_duration(is);

    LOGV("Java_com_kotlin_media_DeviceSurface_ffmpegDuration: %s, duration: %" PRId64"\n", utf8, duration);

    return duration;
}

extern "C" JNIEXPORT jlong JNICALL
Java_me_tang_ffplay_FFplay_currentDuration(
        JNIEnv* env,
        jobject /* this */,
        jlong handle) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);

    int64_t duration = get_current_duration(is);

    LOGV("Java_com_kotlin_media_DeviceSurface_ffmpegCurrentDuration: %s, duration: %" PRId64"\n", utf8, duration);

    return duration;
}

extern "C" JNIEXPORT void JNICALL
Java_me_tang_ffplay_FFplay_seek(
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
Java_me_tang_ffplay_FFplay_setRate(
        JNIEnv* env,
        jobject /* this */,
        jlong handle,
        jint rate) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);

    LOGV("Java_com_kotlin_media_DeviceSurface_ffmpegSetRate: %s, rate: %d\n", utf8, rate);

    set_rate(is, rate);
}


extern "C" JNIEXPORT void JNICALL
Java_me_tang_ffplay_FFplay_setSurface(
        JNIEnv* env,
        jobject /* this */,
        jlong handle,
        jobject surface) {

    VideoState *is = (VideoState *)handle;

    const char *utf8 = stream_filename(is);
    jobject gsurface = (jobject)stream_surface(is);
    jobject new_gsurface = env->NewGlobalRef(surface);

    LOGV("set surface  file %s, old surface %x, new surface %x\n", utf8, gsurface, new_gsurface);

    set_surface(is, new_gsurface);

    if (gsurface)
        env->DeleteGlobalRef(gsurface);
}

