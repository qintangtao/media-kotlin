//
// Created by Administrator on 2022/4/14.
//

#ifndef NDKDEMO_QUEUEWINDOWDATA_H
#define NDKDEMO_QUEUEWINDOWDATA_H

#include <jni.h>
#include "threadsafe_queue.h"
#include "threadsafe_list.h"

class RGBData;
class WindowData;

class QueueWindowData {
public:
    QueueWindowData();
    ~QueueWindowData();

    int init(const char *ip, int port, int test);
    jlong append(JNIEnv *env, jint id, jobject surface);
    void remove(jint id, jlong w);
    void close();

    void run_show_data();

private:
    int init_rgb(const char *ip, int port, int test);
    void close_rgb();

    int start();
    int stop();

private:
    pthread_t m_show_thread;
    bool m_show_exit;

    RGBData *m_rgb_data;

    threadsafe_queue<WindowData *> m_window_queue;
    threadsafe_list<WindowData *> m_window_list;
};


#endif //NDKDEMO_QUEUEWINDOWDATA_H
