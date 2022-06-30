package com.kotlin.media.ui.main

import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import androidx.core.app.ActivityCompat
import com.kotlin.media.R
import com.kotlin.media.databinding.ActivityMainBinding
import com.kotlin.media.ui.edit.EditActivity
import com.kotlin.media.ui.group.VideoGroupActivity
import dagger.hilt.android.AndroidEntryPoint
import me.tang.mvvm.base.BaseActivity
import java.lang.Exception

@AndroidEntryPoint
class MainActivity : BaseActivity<MainViewModel, ActivityMainBinding>() {

    override fun initView(savedInstanceState: Bundle?) {
        Log.d("native-lib", "mBinding is $mBinding")
        mBinding.viewModel = viewModel

        mBinding.swipeRefreshLayout.run {
            setColorSchemeResources(R.color.textColorPrimary)
            setProgressBackgroundColorSchemeResource(R.color.bgColorPrimary)
            setOnRefreshListener { viewModel.refreshVideoList() }
        }

        mBinding.ivAdd.setOnClickListener {
            startActivity(Intent(this, EditActivity::class.java))
            //startActivity(Intent(this, VideoGroupActivity::class.java))
        }
    }

    override fun initData() {
        verifyStoragePermissions(this);
        viewModel.refreshVideoList()
    }

    override fun onLoadCompleted() {
        super.onLoadCompleted()
        mBinding.swipeRefreshLayout.run {
            if (isRefreshing)
                isRefreshing = false
        }
    }

    override fun onStart() {
        super.onStart()
        Log.d("EditViewModel", "onStart")
    }

    override fun onPause() {
        super.onPause()
        Log.d("EditViewModel", "onPause")
    }

    private val PERMISSION_STORAGE = arrayOf(
        android.Manifest.permission.READ_EXTERNAL_STORAGE,
        android.Manifest.permission.WRITE_EXTERNAL_STORAGE
    )

    private fun verifyStoragePermissions(activity: Activity) {
        try {
            val permission = ActivityCompat.checkSelfPermission(activity,
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
            if (permission != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(activity, PERMISSION_STORAGE, 1)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

}
