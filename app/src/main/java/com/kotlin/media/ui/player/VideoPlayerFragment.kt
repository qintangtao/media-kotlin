package com.kotlin.media.ui.player

import android.os.Bundle
import android.widget.SeekBar
import androidx.fragment.app.Fragment
import com.kotlin.media.databinding.FragmentVideoPlayerBinding
import com.kotlin.media.model.bean.Video
import me.tang.mvvm.base.BaseFragment
import me.tang.mvvm.bus.Bus

// TODO: Rename parameter arguments, choose names that match
// the fragment initialization parameters, e.g. ARG_ITEM_NUMBER

/**
 * A simple [Fragment] subclass.
 * Use the [VideoPlayerFragment.newInstance] factory method to
 * create an instance of this fragment.
 */
class VideoPlayerFragment : BaseFragment<VideoPlayerViewModel, FragmentVideoPlayerBinding>() {

    private val rateFragment by lazy { RateListDialogFragment.newInstance() }

    private var _video : Video? = null
    private val video get() = _video!!

    fun setVideo(video: Video) {
        _video = video
    }

    override fun isShareVM(): Boolean = true

    override fun initView(savedInstanceState: Bundle?) {
        super.initView(savedInstanceState)
        mBinding.viewModel = viewModel

        mBinding.volumeUri.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            var lastProgress = 100

            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean)
            { lastProgress = progress }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {}

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                setVolume(lastProgress)
            }
        })

        mBinding.sbSeek.max = Int.MAX_VALUE

        mBinding.sbSeek.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {

            var lastProgress = 100
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean)
            { lastProgress = progress }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                viewModel.updateDuration(true)
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                val duration = mBinding.ptvPlayer.getDuration()
                val x = duration * lastProgress /  Int.MAX_VALUE
                mBinding.ptvPlayer.seek(x)
                viewModel.updateDuration(false)
            }
        })

        mBinding.btnBack.setOnClickListener {
            mBinding.ptvPlayer.fastBack()
        }
        mBinding.btnForward.setOnClickListener {
            mBinding.ptvPlayer.fastForward()
        }

        mBinding.btnInfo.setOnClickListener {
            val duration = mBinding.ptvPlayer.getDuration()
            //var duration : Long = duration2 + (if (duration2 <= Long.MAX_VALUE - 5000) 5000 else 0)
            mBinding.tvTime.text = mBinding.ptvPlayer.formatDuration(duration)

            //val current_duration = mBinding.ptvPlayer.getCurrentDuration()
            //mBinding.tvCurrTime.text = mBinding.ptvPlayer.formatDuration(current_duration)
        }

        mBinding.btnRate.setOnClickListener {
            rateFragment.show(parentFragmentManager)
        }

        Bus.observe<Int>("RATE", this) {
            mBinding.ptvPlayer.setRate(it)
        }
    }

    override fun lazyLoadData() {
        super.lazyLoadData()
        _video?.let {
            viewModel.initData(it)
        }
    }

    fun setVolume(percent: Int) {
        var volume_level: Int
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


}