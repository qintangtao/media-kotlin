//
// Created by Administrator on 2022/4/14.
//

#include "QueueWindowData.h"
#include <unistd.h>
#include "RGBData.h"
#include "WindowData.h"
#include "time_util.h"

#include <android/log.h>
#if 0
#define LOGV(...)
#define LOGE(...)
#else
#define TAG "QueueWindowData"
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
    LOGV("Frame size : %d, %d, %d", sizeof(uint8_t*), sizeof(uint32_t), sizeof(media::Frame));
    init_rgb(ip, port, test);
    return start();
}

jlong QueueWindowData::append(JNIEnv *env, jint id, jobject surface)
{
    WindowData *data = new WindowData();

    jlong window = data->init(env, id, surface);

    std::lock_guard<std::mutex> lk(m_window_mut);

    m_window_list.push_back(data);

    LOGV("append window: id=%d, surface=%d, window=%d, list_size=%d", id, surface, window, m_window_list.size());

    return window;
}

void QueueWindowData::remove(jint id, jlong w)
{
    WindowData *window;
    std::list<WindowData *>::iterator it;

    LOGV("remove window: id=%d, window=%d", id, w);

    std::lock_guard<std::mutex> lk(m_window_mut);
    for (it = m_window_list.begin(); it != m_window_list.end(); it++) {
        window = *it;
        if (window->id() == id) {
            m_window_list.erase(it);
            window->close();
            delete window;
            break;
        }
    }
}

void QueueWindowData::close()
{
    stop();
    close_rgb();
}

void QueueWindowData::run_show_data()
{
    media::Rgb *rgb;
    uint64_t frame_timer;
    uint64_t time;
    uint32_t delay;
    threadsafe_queue<media::Rgb *> &rgb_queue = m_rgb_data->get_rgb_queue();

    delay = 33 * 1000;
    frame_timer = gettime_relative() - delay;


    m_show_exit = false;
    while (!m_show_exit && m_rgb_data) {

        //LOGV("rgb queue count %d", rgb_queue.size());
        while (!m_show_exit && rgb_queue.try_pop(rgb)) {
            //LOGV("rgb queue count %d", rgb_queue.size());


#if 1
            // 同步
            // 还没到下一帧播放时间，则等待
            //LOGV("render sync: start");

            // 下一帧 距离 上一帧 时间
            //delay = 30 * 1000;

            time = gettime_relative();

            while (!m_show_exit && time < (frame_timer + delay)) {
#if 1
                uint64_t diff = frame_timer + delay - time;
                //LOGV("render sync: frame_timer=%lld, last_timer=%lld, timer=%lld, delay=%d, diff=%lld",
                //     frame_timer, (frame_timer + delay), time, delay, diff);
                if (diff > 0)
                    usleep(diff);
#else
                if (p_frame_data)
                    render(p_frame_data);
#endif
                time = gettime_relative();
            }

            frame_timer = time;

            //LOGV("render sync: end");
#endif

            render(rgb->data, rgb->size);

            media::rgb_free(rgb);
        }

#if 1
        usleep(1000);
#else
        if (p_frame_data)
            render(p_frame_data);
        else
            usleep(10);
#endif
    }
}

void QueueWindowData::render(uint8_t *data, size_t size)
{
    WindowData *window;
    std::list<WindowData *>::iterator it;

    std::lock_guard<std::mutex> lk(m_window_mut);
    for (it = m_window_list.begin(); it != m_window_list.end(); it++) {
        window = *it;
        window->render(data, size);
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