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
                //val attenuation = 100 - lastProgress
                //val millibel = attenuation * -50
                setVolume(lastProgress)
            }
        })

        mBinding.sbSeek.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {

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

            }
        })

        mBinding.btnBack.setOnClickListener {
            mBinding.ptvPlayer.fastBack()
        }
        mBinding.btnForward.setOnClickListener {
            mBinding.ptvPlayer.fastForward()
        }
    }

    fun setVolume(percent: Int) {
        var volume_level  = 0
        if (percent > 30) {
            volume_level = (100 - percent) * -20
        } else if (percent > 25) {
            volume_level = (100 - percent) * -22
        } else if (percent > 20) {
            volume_level = (100 - percent) * -25
        } else if (percent > 15) {
            volume_level = (100 - percent) * -28
        } else if (percent > 10) {
            volume_level = (100 - percent) * -30
        } else if (percent > 5) {
            volume_level = (100 - percent) * -34
        } else if (percent > 3) {
            volume_level = (100 - percent) * -37
        } else if (percent > 0) {
            volume_level = (100 - percent) * -40
        } else {
            volume_level = (100 - percent) * -100
        }
        mBinding.ptvPlayer.setVolume(volume_level)
    }


    override fun initData() {
        (intent.getParcelableExtra(PARAM_VIDEO) as? Video)?.let {
            viewModel.setBean(it)
        }
    }

}