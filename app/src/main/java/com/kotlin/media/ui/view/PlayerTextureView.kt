package com.kotlin.media.ui.view

import android.content.Context
import android.graphics.SurfaceTexture
import android.util.AttributeSet
import android.util.Log
import android.view.Surface
import android.view.TextureView
import com.kotlin.media.DeviceSurface
import java.time.Duration

class PlayerTextureView  : TextureView, TextureView.SurfaceTextureListener {

    enum class PlayState(val code: Int, val msg: String) {
        UnconnectedState(0, "没有播放"),
        ConnectingState(1, "连接中"),
        ConnectedState(2, "连接上"),
        ParsingState(3, "解析码流信息"),
        PlayingState(4, "播放中"),
        PauseState(4, "暂停中"),
        ClosingState(5, "关闭中");
    }

    enum class ffEvent(val code: Long){
        FF_EVENT_TOOGLE_PAUSE(0),      //暂停
        FF_EVENT_TOOGLE_MUTE(1),       //静音
        FF_EVENT_INC_VOLUME(2),        //静音加
        FF_EVENT_DEC_VOLUME(3),        //静音减
        FF_EVENT_NEXT_FRAME(4),         //下一帧
        FF_EVENT_FAST_BACK(5),         //后退
        FF_EVENT_FAST_FORWARD(6);      //快进
    };

    var mHandler: Long = 0
    var mSurface: Surface? = null

    constructor(context: Context): this(context, null)
    constructor(context: Context, attrs: AttributeSet?): this(context, attrs, 0)
    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int): super(context, attrs, defStyleAttr) {
        init(context, attrs)
    }

    fun start(filename: String) : Boolean{
        Log.d("native-lib", "start: $filename")
        mSurface?.let {
            Log.d("native-lib", "start2: $filename")
            mHandler = DeviceSurface.get().ffmpegOpen(filename, mSurface!!)
            Log.d("native-lib", "start3: $filename mHandler: $mHandler")
            return true
        }
        return false
    }

    fun stop() {
        Log.d("native-lib", "stop: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            Log.d("native-lib", "stop2: $mHandler")
            DeviceSurface.get().ffmpegClose(mHandler)
            mHandler = 0
        }
    }

    fun tooglePause() {
        Log.d("native-lib", "tooglePause: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            DeviceSurface.get().ffmpegSendEvent(mHandler, ffEvent.FF_EVENT_TOOGLE_PAUSE.code)
        }
    }

    fun toogleMute() {
        Log.d("native-lib", "toogleMute: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            DeviceSurface.get().ffmpegSendEvent(mHandler, ffEvent.FF_EVENT_TOOGLE_MUTE.code)
        }
    }

    fun fastBack() {
        Log.d("native-lib", "fastBack: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            DeviceSurface.get().ffmpegSendEvent(mHandler, ffEvent.FF_EVENT_FAST_BACK.code)
        }
    }

    fun fastForward() {
        Log.d("native-lib", "fastFoward: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            DeviceSurface.get().ffmpegSendEvent(mHandler, ffEvent.FF_EVENT_FAST_FORWARD.code)
        }
    }

    fun setVolume(volume: Int) {
        Log.d("native-lib", "setVolume: $mHandler $volume")
        if (mHandler.compareTo(0) != 0) {
            DeviceSurface.get().ffmpegSetVolume(mHandler, volume.toLong())
        }
    }

    fun getDuration() : Long {
        Log.d("native-lib", "getDuration: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            return DeviceSurface.get().ffmpegDuration(mHandler)
        }
        return 0
    }

    fun getCurrentDuration() : Long {
        Log.d("native-lib", "getCurrentDuration: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            return DeviceSurface.get().ffmpegCurrentDuration(mHandler)
        }
        return 0
    }

    fun seek(pos: Long) {
        Log.d("native-lib", "seek: $mHandler $pos")
        if (mHandler.compareTo(0) != 0) {
            DeviceSurface.get().ffmpegSeek(mHandler, pos)
        }
    }

    fun setRate(rate: Int) {
        Log.d("native-lib", "setRate: $mHandler $rate")
        if (mHandler.compareTo(0) != 0) {
            DeviceSurface.get().ffmpegSetRate(mHandler, rate)
        }
    }

    fun formatDuration(duration: Long) : String {
        val AV_TIME_BASE = 1000000
        var hours: Int
        var mins: Int
        var secs: Int
        var us: Int
        var msecs: Int
        secs  = (duration / AV_TIME_BASE).toInt()
        us    = (duration % AV_TIME_BASE).toInt()
        mins  = secs / 60
        secs %= 60
        hours = mins / 60
        mins %= 60
        msecs = (100 * us) / AV_TIME_BASE

        val _hours = String.format("%02d", hours)
        val _mins = String.format("%02d", mins)
        val _secs = String.format("%02d", secs)
        val _msecs = String.format("%02d", msecs)

        return  "$_hours:$_mins:$_secs.$_msecs"
    }

    fun state() : PlayState {
        return PlayState.UnconnectedState
    }

    fun init(context: Context, attrs: AttributeSet?) {
        surfaceTextureListener = this
    }

    fun create(surfaceTexture: SurfaceTexture) {
        mSurface = Surface(surfaceTexture)
    }

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        Log.d("native-lib", "onSurfaceTextureAvailable")
        create(surface)
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
        Log.d("native-lib", "onSurfaceTextureSizeChanged")
    }

    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        Log.d("native-lib", "onSurfaceTextureDestroyed")
        stop()
        return true
    }

    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {
        Log.d("native-lib", "onSurfaceTextureUpdated")
    }

}