package com.kotlin.media.ui.video.detail

import android.util.Log
import android.view.View
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.kotlin.media.DeviceSurface
import com.kotlin.media.R
import com.kotlin.media.model.bean.Video
import com.kotlin.media.ui.view.PlayerTextureView
import com.kotlin.mvvm.base.BaseViewModel
import com.kotlin.mvvm.base.OnItemClickListener

class DetailViewModel : BaseViewModel() {

    val listenner = object : OnItemClickListener<Video> {
        override fun onClick(view: View, item: Video) {
            when(view.id) {
                R.id.ivPlay -> {
                    var ptvPlayer = (view.parent as View).findViewById<PlayerTextureView>(R.id.ptv_player)

                    Log.d("native-lib", "DetailViewModel.ivPlay: " + ptvPlayer)

                    ptvPlayer?.let {
                        if (!item.play) ptvPlayer.start(item.url) else ptvPlayer.stop()
                        item.play = !item.play
                        _itemBean.value = item
                    }
                }
                R.id.ivPaused -> {
                    var ptvPlayer = (view.parent as View).findViewById<PlayerTextureView>(R.id.ptv_player)

                    Log.d("native-lib", "DetailViewModel.ivPaused: " + ptvPlayer)

                    ptvPlayer?.let {
                        ptvPlayer.tooglePause()
                    }
                }
                R.id.ivMute -> {
                    var ptvPlayer = (view.parent as View).findViewById<PlayerTextureView>(R.id.ptv_player)

                    Log.d("native-lib", "DetailViewModel.ivPaused: " + ptvPlayer)

                    ptvPlayer?.let {
                        ptvPlayer.toogleMute()
                    }
                }
            }
        }
    }

    private val _itemBean = MutableLiveData<Video>()

    val itemBean: LiveData<Video> = _itemBean

    fun setBean(item : Video) {
        _itemBean.value = item
    }
}