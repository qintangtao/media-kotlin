//
// Created by Administrator on 2022/4/14.
//

#include "WindowData.h"
#include <string.h>
#include <android/native_window_jni.h>

#include <android/log.h>
#define TAG "WindowData"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)


WindowData::WindowData()
{
    m_id = 0;
    m_window = NULL;
}

WindowData::~WindowData()
{
    close();
}

jlong WindowData::init(JNIEnv *env, jint id, jobject surface)
{
    close();

    m_id = id;

    m_window = ANativeWindow_fromSurface(env, surface);

    geometry(1920, 1080, WINDOW_FORMAT_RGB_565);

    LOGV("ANativeWindow_fromSurface %d", m_window);

    return (jlong)m_window;
}

void WindowData::geometry(int32_t width, int32_t height, int32_t format)
{
    if (!m_window) {
        LOGV("render window is null");
        return;
    }

    do {
        int32_t old_width = ANativeWindow_getWidth(m_window);
        if (old_width != width)
            break;

        int32_t old_height = ANativeWindow_getHeight(m_window);
        if (old_height != height)
            break;

        int32_t old_format = ANativeWindow_getFormat(m_window);
        if (old_format != format)
            break;

        return;
    } while(1);

    ANativeWindow_setBuffersGeometry(m_window, width, height, format);
}

void WindowData::close()
{
    if (m_window) {
        LOGV("ANativeWindow_release %d", m_window);
        ANativeWindow_release(m_window);
        m_window = NULL;
    }

    m_id = 0;
}

int WindowData::render(uint8_t *data, int size)
{
    //LOGV("render window id:%d len:%d", m_id, size);

    if (!m_window) {
        LOGV("render window is null");
        return -1;
    }

    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(m_window, &window_buffer, 0)) {
        LOGV("lock window failed %d", m_window);
        ANativeWindow_release(m_window);
        m_window = 0;
        return -1;
    }

    memcpy(window_buffer.bits, data, size);

    ANativeWindow_unlockAndPost(m_window);

    return 0;
}