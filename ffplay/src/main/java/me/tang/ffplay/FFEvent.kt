package me.tang.ffplay

enum class FFEvent(val code: Long){
    FF_EVENT_TOOGLE_PAUSE(0),      //暂停
    FF_EVENT_TOOGLE_MUTE(1),       //静音
    FF_EVENT_INC_VOLUME(2),        //静音加
    FF_EVENT_DEC_VOLUME(3),        //静音减
    FF_EVENT_NEXT_FRAME(4),         //下一帧
    FF_EVENT_FAST_BACK(5),         //后退
    FF_EVENT_FAST_FORWARD(6);      //快进
}
