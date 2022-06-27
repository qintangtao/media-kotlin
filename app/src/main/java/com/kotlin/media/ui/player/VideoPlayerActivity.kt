package com.kotlin.media.ui.player

import android.os.Bundle
import android.util.Log
import com.kotlin.media.R
import com.kotlin.media.databinding.ActivityVideoPlayerBinding
import com.kotlin.media.model.bean.Video
import com.kotlin.mvvm.base.BaseActivity

class VideoPlayerActivity  : BaseActivity<VideoPlayerViewModel, ActivityVideoPlayerBinding>() {

    companion object {
        const val PARAM_VIDEO = "param_video"
    }

    override fun initView(savedInstanceState: Bundle?) {
        (intent.getParcelableExtra(PARAM_VIDEO) as? Video)?.let {
            Log.d("native-lib", "$PARAM_VIDEO: $it")

            val fragment: VideoPlayerFragment? = supportFragmentManager.findFragmentById(R.id.video_player_fragment) as? VideoPlayerFragment
            Log.d("native-lib", "fragment: $fragment")

            fragment?.run {
                setVideo(it)
            }
        }
    }

    override fun initData() {
    }
}