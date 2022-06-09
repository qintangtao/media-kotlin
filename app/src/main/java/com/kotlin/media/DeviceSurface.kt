package com.kotlin.media

import android.view.Surface


class DeviceSurface {

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    external fun init(ip: String, port: Int, demo: Int)

    external fun close()

    external fun append(id: Int, surface: Surface) : Long

    external fun remove(id: Int, long: Long)

    external fun openFfmpeg(filename: String)

    external fun closeFfmpeg()

    external fun ffmpegOpen(filename: String,  surface: Surface) : Long

    external fun ffmpegClose(handle: Long)

    external fun ffmpegSendEvent(handle: Long, code: Long)

    external fun ffmpegSetVolume(handle: Long, volume: Long)



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
