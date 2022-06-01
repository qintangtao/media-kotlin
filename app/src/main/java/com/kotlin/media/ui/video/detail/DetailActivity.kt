package com.kotlin.media.ui.video.detail

import android.os.Bundle
import android.util.Log
import com.kotlin.media.databinding.ActivityDetailBinding
import com.kotlin.mvvm.base.BaseActivity
import com.kotlin.media.model.bean.Video

class DetailActivity : BaseActivity<DetailViewModel, ActivityDetailBinding>() {

    companion object {
        const val PARAM_VIDEO = "param_video"
    }

    override fun initView(savedInstanceState: Bundle?) {
        Log.d("native-lib", "mBinding is $mBinding")
        mBinding.viewModel = viewModel
    }

    override fun initData() {
        (intent.getParcelableExtra(PARAM_VIDEO) as? Video)?.let {
            viewModel.setBean(it)
        }
    }

}