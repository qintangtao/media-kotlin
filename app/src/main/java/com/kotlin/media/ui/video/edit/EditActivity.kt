package com.kotlin.media.ui.video.edit

import android.os.Bundle
import android.util.Log
import com.blankj.utilcode.util.ToastUtils
import com.kotlin.media.data.local.VideoDao
import com.kotlin.media.databinding.ActivityEditBinding
import com.kotlin.mvvm.base.BaseActivity
import com.kotlin.mvvm.event.Message
import com.kotlin.mvvm.network.RESULT
import dagger.hilt.android.AndroidEntryPoint
import javax.inject.Inject

@AndroidEntryPoint
class EditActivity : BaseActivity<EditViewModel, ActivityEditBinding>() {

    @Inject
    lateinit var videoDao: VideoDao

    override fun initView(savedInstanceState: Bundle?) {
        mBinding.viewModel = viewModel
        mBinding.ivClose.setOnClickListener { finish() }

        mBinding.run {
            tilName.isStartIconVisible = false
        }
    }

    override fun initData() {

        Log.d("EditViewModel", "videoDao1:" + videoDao)

        //viewModel.saveVideo2(videoDao, Video(name = "test", url = "url"))

    }

    override fun onLoadResult(code: Int) {
        when(code) {
            RESULT.END.code -> {
                ToastUtils.showLong("添加成功")
                finish()
            }
            else -> super.onLoadResult(code)
        }
    }

    override fun onLoadEvent(msg: Message) {
        viewModel.urlError.value = msg.msg
    }

}