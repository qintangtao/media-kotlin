package com.kotlin.media.ui.player

import android.os.Bundle
import com.kotlin.media.R
import com.kotlin.media.databinding.ActivityVideoPlayerBinding
import com.kotlin.media.model.bean.Video
import me.tang.mvvm.base.BaseActivity

class VideoPlayerActivity  : BaseActivity<VideoPlayerViewModel, ActivityVideoPlayerBinding>() {

    companion object {
        const val PARAM_VIDEO = "param_video"
    }

    private var fragment: VideoPlayerFragment? = null
    private var video: Video? = null

    override fun initView(savedInstanceState: Bundle?) {

        fragment = supportFragmentManager.findFragmentById(R.id.video_player_fragment) as? VideoPlayerFragment
        check(fragment!=null) { "video player fragment not found" }

        video = intent.getParcelableExtra(PARAM_VIDEO) as? Video
        check(video!=null) { "param video is not set" }

        fragment?.setVideo(video!!)

    }

    override fun initData() {
    }
}