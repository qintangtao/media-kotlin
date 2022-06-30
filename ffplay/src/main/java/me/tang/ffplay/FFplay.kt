package me.tang.ffplay

import android.view.Surface

class FFplay {


    external fun open(filename: String,  surface: Surface) : Long

    external fun close(handle: Long)

    external fun sendEvent(handle: Long, code: Long)

    external fun setVolume(handle: Long, volume: Long)

    external fun duration(handle: Long) : Long

    external fun currentDuration(handle: Long) : Long

    external fun seek(handle: Long, pos: Long)

    external fun setRate(handle: Long, rate: Int)

    companion object {
        init {
            System.loadLibrary("ffplay")
        }

        private var instance: FFplay? = null
            get() {
                if (field != null) {
                    return field
                }
                return FFplay()
            }
        fun get(): FFplay = instance!!
    }
}
