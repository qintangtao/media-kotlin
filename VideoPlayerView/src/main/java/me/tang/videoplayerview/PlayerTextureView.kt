package me.tang.videoplayerview

import android.content.Context
import android.graphics.Outline
import android.graphics.Rect
import android.graphics.SurfaceTexture
import android.util.AttributeSet
import android.util.Log
import android.view.Surface
import android.view.TextureView
import android.view.View
import android.view.ViewOutlineProvider
import com.blankj.utilcode.util.SizeUtils
import me.tang.ffplay.FFEvent
import me.tang.ffplay.FFplay

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

    private val TAG = PlayerTextureView::class.java.name
    private var mHandler: Long = 0
    private var mSurface: Surface? = null

    constructor(context: Context): this(context, null)
    constructor(context: Context, attrs: AttributeSet?): this(context, attrs, 0)
    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int): super(context, attrs, defStyleAttr) {
        init(context, attrs)
    }

    fun isPlaying() : Boolean = mHandler.compareTo(0) != 0

    fun start(filename: String) : Boolean{
        Log.d("TAG", "start: $filename, address:$this")
        mSurface?.let {
            mHandler = FFplay.get().open(filename, mSurface!!)
            return true
        }
        return false
    }

    fun stop() {
        Log.d("TAG", "stop: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            FFplay.get().close(mHandler)
            mHandler = 0
        }
    }

    fun tooglePause() {
        Log.d("TAG", "tooglePause: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            FFplay.get().sendEvent(mHandler, FFEvent.FF_EVENT_TOOGLE_PAUSE.code)
        }
    }

    fun toogleMute() {
        Log.d("TAG", "toogleMute: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            FFplay.get().sendEvent(mHandler, FFEvent.FF_EVENT_TOOGLE_MUTE.code)
        }
    }

    fun fastBack() {
        Log.d("TAG", "fastBack: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            FFplay.get().sendEvent(mHandler, FFEvent.FF_EVENT_FAST_BACK.code)
        }
    }

    fun fastForward() {
        Log.d("TAG", "fastFoward: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            FFplay.get().sendEvent(mHandler, FFEvent.FF_EVENT_FAST_FORWARD.code)
        }
    }

    fun setVolume(volume: Int) {
        Log.d("TAG", "setVolume: $mHandler $volume")
        if (mHandler.compareTo(0) != 0) {
            FFplay.get().setVolume(mHandler, volume.toLong())
        }
    }

    fun getDuration() : Long {
        Log.d("TAG", "getDuration: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            return FFplay.get().duration(mHandler)
        }
        return 0
    }

    fun getCurrentDuration() : Long {
        Log.d("TAG", "getCurrentDuration: $mHandler")
        if (mHandler.compareTo(0) != 0) {
            return FFplay.get().currentDuration(mHandler)
        }
        return 0
    }

    fun seek(pos: Long) {
        Log.d("TAG", "seek: $mHandler $pos")
        if (mHandler.compareTo(0) != 0) {
            FFplay.get().seek(mHandler, pos)
        }
    }

    fun setRate(rate: Int) {
        Log.d("TAG", "setRate: $mHandler $rate")
        if (mHandler.compareTo(0) != 0) {
            FFplay.get().setRate(mHandler, rate)
        }
    }

    fun formatDuration(duration: Long) : String {
        val AV_TIME_BASE = 1000000
        var mins: Int
        var secs: Int = (duration / AV_TIME_BASE).toInt()
        val us: Int = (duration % AV_TIME_BASE).toInt()
        mins  = secs / 60
        secs %= 60
        val hours = mins / 60
        mins %= 60
        val msecs = (100 * us) / AV_TIME_BASE

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
        outlineProvider = TextureViewOutlineProvider(SizeUtils.dp2px(12.0f).toFloat())
        clipToOutline = true;
    }

    fun create(surfaceTexture: SurfaceTexture) {
        mSurface = Surface(surfaceTexture)
    }

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        Log.d("TAG", "onSurfaceTextureAvailable width:$width, height:$height, address:$this")
        create(surface)
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
        Log.d("TAG", "onSurfaceTextureSizeChanged width:$width, height:$height")
    }

    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        Log.d("TAG", "onSurfaceTextureDestroyed, address:$this")
        stop()
        return true
    }

    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {
        Log.d("TAG", "onSurfaceTextureUpdated")
    }


    class TextureViewOutlineProvider(var radius: Float = 0.0f) : ViewOutlineProvider() {
        override fun getOutline(view: View?, outline: Outline?) {
            view?.let {
                val rect = Rect()
                it.getGlobalVisibleRect(rect)
                val leftMargin = 0
                val topMargin = 0
                val selfRect = Rect(
                    leftMargin, topMargin,
                    rect.right - rect.left - leftMargin, rect.bottom - rect.top - topMargin
                )
                outline?.setRoundRect(selfRect, this.radius)
            }
        }
    }
}