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
import com.kotlin.mvvm.BR
import com.kotlin.mvvm.base.BaseViewModel
import com.kotlin.mvvm.base.OnItemClickListener
import com.kotlin.mvvm.bus.Bus
import com.kotlin.mvvm.event.Message
import com.kotlin.mvvm.network.ExceptionHandle
import com.kotlin.mvvm.network.RESULT
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.launch
import me.tatarka.bindingcollectionadapter2.ItemBinding

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


    private val _rate = MutableLiveData<String>()

    val rate: LiveData<String> = _rate

    val listenner2 = object : OnItemClickListener<String> {
        override fun onClick(view: View, item: String) {
            Log.d("native-lib", "onClick is $item")
            Bus.post("HIDDEN_RATE", item)
            setRate(item)
        }
    }

    private val _items = MutableLiveData<MutableList<String>>()
    val items: LiveData<MutableList<String>> = _items
    val itemBinding = ItemBinding.of<String>(BR.itemBean, R.layout.fragment_rate_list_dialog_list_dialog_item)
        .bindExtra(BR.listenner, listenner2)

    var isRefreshDurationExit = false
    var isRefreshDurationPaused = false

    private val rates = listOf<String>("4X", "3X", "2X", "1.25x", "正常", "0.5X")
    private val ratev = listOf<Int>(2000, 1750, 1500, 1250, 1000, 500)


    fun initData() {
        _items.value = rates.toMutableList()
        _rate.value = "正常"
    }

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
                    delay(200)
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

    fun setRate(rate: String) {
        _rate.value = rate
        Bus.post("RATE", ratev.get(rates.indexOf(rate)))
    }
}