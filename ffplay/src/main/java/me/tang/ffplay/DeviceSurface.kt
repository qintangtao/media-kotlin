package me.tang.ffplay

import android.view.Surface

class DeviceSurface {

    external fun ffmpegOpen(filename: String,  surface: Surface) : Long

    external fun ffmpegClose(handle: Long)

    external fun ffmpegSendEvent(handle: Long, code: Long)

    external fun ffmpegSetVolume(handle: Long, volume: Long)

    external fun ffmpegDuration(handle: Long) : Long

    external fun ffmpegCurrentDuration(handle: Long) : Long

    external fun ffmpegSeek(handle: Long, pos: Long)

    external fun ffmpegSetRate(handle: Long, rate: Int)


    companion object {
        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }

        private var instance: DeviceSurface? = null
            get() {
                if (field != null) {
                    return field
                }
                return DeviceSurface()
            }
        fun get(): DeviceSurface {
            return instance!!
        }
    }
}
