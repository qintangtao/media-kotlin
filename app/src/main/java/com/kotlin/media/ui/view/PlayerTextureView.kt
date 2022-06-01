package com.kotlin.media.ui.view

import android.content.Context
import android.graphics.SurfaceTexture
import android.util.AttributeSet
import android.util.Log
import android.view.Surface
import android.view.TextureView
import com.kotlin.media.DeviceSurface

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

    var mHandler: Long = 0
    var mSurface: Surface? = null

    constructor(context: Context): this(context, null)
    constructor(context: Context, attrs: AttributeSet?): this(context, attrs, 0)
    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int): super(context, attrs, defStyleAttr) {
        init(context, attrs)
    }

    fun start(filename: String) : Boolean{
        Log.d("native-lib", "start: " + filename)
        mSurface?.let {
            Log.d("native-lib", "start2: " + filename)
            mHandler = DeviceSurface.get().ffmpegOpen(filename, mSurface!!)
            Log.d("native-lib", "start3: " + filename + " mHandler: " + mHandler)
            return true
        }
        return false
    }

    fun stop() {
        Log.d("native-lib", "stop: " + mHandler)
        if (mHandler.compareTo(0) != 0) {
            Log.d("native-lib", "stop2: " + mHandler)
            DeviceSurface.get().ffmpegClose(mHandler)
            mHandler = 0
        }
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