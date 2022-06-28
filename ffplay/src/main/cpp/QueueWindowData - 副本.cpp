//
// Created by Administrator on 2022/4/14.
//

#include "QueueWindowData.h"
#include <unistd.h>
#include "RGBData.h"
#include "WindowData.h"

#include <android/log.h>
#define TAG "QueueWindowData"
#if 1
#define LOGV(...)
#define LOGE(...)
#else
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#endif
void *run_show(void *arg)
{
    LOGV("enter show thread: ");
    ((QueueWindowData*)arg)->run_show_data();
    LOGV("level show thread: ");
    return NULL;
}

QueueWindowData::QueueWindowData()
{
    m_show_thread = NULL;
    m_show_exit = false;
    m_rgb_data = NULL;
}
QueueWindowData::~QueueWindowData()
{
    close();
}

int QueueWindowData::init(const char *ip, int port, int test)
{
    init_rgb(ip, port, test);
    return start();
}

jlong QueueWindowData::append(JNIEnv *env, jint id, jobject surface)
{
    WindowData *data = new WindowData();

    jlong s = data->init(env, id, surface);

    m_window_queue.push(data);

    return s;
}

void QueueWindowData::remove(jint id, jlong w)
{
    WindowData *window;
    threadsafe_queue<WindowData *> window_queue;

    while (m_window_queue.try_pop(window)) {

        if (window->id() == id)
        {
            window->close();
            delete window;
            continue;
        }

        window_queue.push(window);
    }

    while (window_queue.try_pop(window)) {
        m_window_queue.push(window);
    }
}
void QueueWindowData::close()
{
    stop();
    close_rgb();
}

void QueueWindowData::run_show_data()
{
    uint8_t *buf;
    uint8_t *p_data;
    ssize_t len;
    WindowData *window;
    threadsafe_queue<WindowData *> window_queue;
    threadsafe_queue<uint8_t *> &queue = m_rgb_data->get_rgb_queue();

    m_show_exit = false;
    while (!m_show_exit && m_rgb_data) {

        LOGV("rgb queue count %d", queue.size());
        while (queue.try_pop(buf)) {
            len = *((ssize_t*)buf);
            p_data = buf + 4;

            while (m_window_queue.try_pop(window)) {
                window->render(p_data, len);
                window_queue.push(window);
            }

            while (window_queue.try_pop(window)) {
                m_window_queue.push(window);
            }

            free(buf);
        }

        usleep(10);
    }
}


int QueueWindowData::init_rgb(const char *ip, int port, int test)
{
    close_rgb();

    m_rgb_data = new RGBData();
    m_rgb_data->init(ip, port, test);

    return 0;
}

void QueueWindowData::close_rgb()
{
    if (m_rgb_data)
    {
        m_rgb_data->close();
        delete m_rgb_data;
        m_rgb_data = NULL;
    }
}

int QueueWindowData::start()
{
    int err;
    err = pthread_create(&m_show_thread, NULL, run_show, this);
    if (err != 0) {
        LOGE("can't create show thread: %s\n", strerror(err));
        m_show_thread = 0;
        return err;
    }

    LOGV("start show thread: %d\n", m_show_thread);

    return err;
}

int QueueWindowData::stop()
{
    int err = 0;

    if (0 != m_show_thread) {
        m_show_exit = true;
        err = pthread_join(m_show_thread, 0);
        LOGV("stop show thread: %d\n", m_show_thread);
        m_show_thread = 0;
    }

    return err;
}