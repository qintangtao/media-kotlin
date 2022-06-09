package com.kotlin.media.ui.video.detail

import android.os.Bundle
import android.util.Log
import android.widget.SeekBar
import com.kotlin.media.databinding.ActivityDetailBinding
import com.kotlin.media.model.bean.Video
import com.kotlin.mvvm.base.BaseActivity

class DetailActivity : BaseActivity<DetailViewModel, ActivityDetailBinding>() {

    companion object {
        const val PARAM_VIDEO = "param_video"
    }

    override fun initView(savedInstanceState: Bundle?) {
        Log.d("native-lib", "mBinding is $mBinding")
        mBinding.viewModel = viewModel

        mBinding.volumeUri.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {

            var lastProgress = 100

            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean)
            {
                lastProgress = progress
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?)
            {

            }

            override fun onStopTrackingTouch(seekBar: SeekBar?)
            {
                val attenuation = 100 - lastProgress
                val millibel = attenuation * -50
                mBinding.ptvPlayer.setVolume(millibel)

            }
        })
    }

    override fun initData() {
        (intent.getParcelableExtra(PARAM_VIDEO) as? Video)?.let {
            viewModel.setBean(it)
        }
    }

}