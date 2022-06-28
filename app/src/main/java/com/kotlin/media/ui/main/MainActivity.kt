package com.kotlin.media.ui.main

import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import androidx.core.app.ActivityCompat
import com.kotlin.media.R
import com.kotlin.media.databinding.ActivityMainBinding
import com.kotlin.media.ui.group.VideoGroupActivity
import com.kotlin.mvvm.base.BaseActivity
import dagger.hilt.android.AndroidEntryPoint
import java.lang.Exception

@AndroidEntryPoint
class MainActivity : BaseActivity<MainViewModel, ActivityMainBinding>() {

    override fun initView(savedInstanceState: Bundle?) {
        Log.d("native-lib", "mBinding is $mBinding")
        mBinding.viewModel = viewModel

        mBinding.run {
            /*
            recyclerView.run {
                setPullRefreshEnabled(false)
                setLoadingMoreEnabled(false)
                setLoadingMoreProgressStyle(ProgressStyle.Pacman)
                setLoadingListener(object : XRecyclerView.LoadingListener {
                    override fun onRefresh() {
                    }

                    override fun onLoadMore() {

                    }
                })
            }*/

            // 不复用item
            //recyclerView.recycledViewPool.setMaxRecycledViews(R.layout.item_video_player, 4)
            //recyclerView.setItemViewCacheSize(0)
        }

        //mBinding.ivAdd.setOnClickListener { startActivity(Intent(this, EditActivity::class.java))  }
        mBinding.ivAdd.setOnClickListener { startActivity(Intent(this, VideoGroupActivity::class.java))  }

    }

    override fun initData() {
        verifyStoragePermissions(this);
        viewModel.refreshVideoList()
    }

    override fun onStart() {
        super.onStart()
        Log.d("EditViewModel", "onStart")

        //DeviceSurface.get().pauseFfmpeg()
    }

    override fun onPause() {
        super.onPause()
        Log.d("EditViewModel", "onPause")

        //DeviceSurface.get().pauseFfmpeg();
    }

    private var PERMISSION_STORAGE = arrayOf(
        android.Manifest.permission.READ_EXTERNAL_STORAGE,
        android.Manifest.permission.WRITE_EXTERNAL_STORAGE
    )

    private fun verifyStoragePermissions(activity: Activity) {
        try {
            var permission = ActivityCompat.checkSelfPermission(activity,
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
            if (permission != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(activity, PERMISSION_STORAGE, 1)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

}
