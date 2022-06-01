package com.kotlin.media.ui.video.detail

import android.view.View
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.kotlin.media.DeviceSurface
import com.kotlin.media.R
import com.kotlin.media.model.bean.Video
import com.kotlin.mvvm.base.BaseViewModel
import com.kotlin.mvvm.base.OnItemClickListener

class DetailViewModel : BaseViewModel() {

    val listenner = object : OnItemClickListener<Video> {
        override fun onClick(view: View, item: Video) {
            when(view.id) {
                R.id.ivPlay ->
                {
                    if (item.play)
                        DeviceSurface.get().closeFfmpeg()
                    else
                        DeviceSurface.get().openFfmpeg(item.url)

                    item.play = !item.play
                    _itemBean.value = item
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