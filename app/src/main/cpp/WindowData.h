//
// Created by Administrator on 2022/4/14.
//

#ifndef NDKDEMO_WINDOWDATA_H
#define NDKDEMO_WINDOWDATA_H

#include <jni.h>

class ANativeWindow;

class WindowData {
public:
    WindowData();
    ~WindowData();

    jlong init(JNIEnv *env, jint id, jobject surface);

    void geometry(int32_t width, int32_t height, int32_t format);

    void close();

    int render(uint8_t *data, int size);

    inline int id()
    { return m_id; }

private:
    int m_id;
    ANativeWindow *m_window;
};


#endif //NDKDEMO_WINDOWDATA_H
