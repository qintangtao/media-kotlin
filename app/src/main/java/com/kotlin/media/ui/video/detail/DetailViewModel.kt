package com.kotlin.media.ui.video.detail

import android.util.Log
import android.view.View
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.viewModelScope
import com.kotlin.media.DeviceSurface
import com.kotlin.media.R
import com.kotlin.media.model.bean.Video
import com.kotlin.media.ui.view.PlayerTextureView
import com.kotlin.mvvm.base.BaseViewModel
import com.kotlin.mvvm.base.OnItemClickListener
import com.kotlin.mvvm.event.Message
import com.kotlin.mvvm.network.ExceptionHandle
import com.kotlin.mvvm.network.RESULT
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.launch

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
                        if (item.play)
                            refreshDuration(it)
                        else
                            isRefreshDurationExit = true
                    }
                }
                R.id.ivPaused -> {
                    var ptvPlayer = (view.parent as View).findViewById<PlayerTextureView>(R.id.ptv_player)

                    Log.d("native-lib", "DetailViewModel.ivPaused: " + ptvPlayer)

                    ptvPlayer?.let {
                        ptvPlayer.tooglePause()
                        isRefreshDurationPaused = !isRefreshDurationPaused
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

    private val _progress = MutableLiveData<Int>()

    val progress: LiveData<Int> = _progress

    private val _currentDuration = MutableLiveData<String>()

    val currentDuration: LiveData<String> = _currentDuration

    private val _itemBean = MutableLiveData<Video>()

    val itemBean: LiveData<Video> = _itemBean

    fun setBean(item : Video) {
        _itemBean.value = item
    }

    var isRefreshDurationExit = false
    var isRefreshDurationPaused = false

    fun updateDuration(paused: Boolean) {
        isRefreshDurationPaused = paused
    }

    fun refreshDuration(ptvPlayer: PlayerTextureView) {
        isRefreshDurationExit = false
        isRefreshDurationPaused = false
        viewModelScope.launch {
            flow {
                while (!isRefreshDurationExit) {

                    if (isRefreshDurationPaused) {
                        delay(100)
                        continue
                    }

                    val duration = ptvPlayer.getCurrentDuration()
                    emit(duration)
                    delay(100)
                }
            }
            .flowOn(Dispatchers.IO)
            .collect {
                //updateDuration()
                val duration = ptvPlayer.getDuration()
                val current_duration = it
                if (duration.compareTo(0) != 0) {
                    val x = current_duration *  Int.MAX_VALUE
                    val y = x / duration
                    _progress.value = y.toInt()
                }
                _currentDuration.value = ptvPlayer.formatDuration(current_duration)
            }
        }
    }

}