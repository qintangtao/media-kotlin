package com.kotlin.media.ui.player

import android.util.Log
import android.view.View
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.viewModelScope
import com.kotlin.media.R
import com.kotlin.media.model.bean.Video
import me.tang.mvvm.BR
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.launch
import me.tang.mvvm.base.BaseViewModel
import me.tang.mvvm.base.OnItemClickListener
import me.tang.mvvm.bus.Bus
import me.tatarka.bindingcollectionadapter2.ItemBinding
import kotlinx.coroutines.flow.collect
import me.tang.videoplayerview.PlayerTextureView

class VideoPlayerViewModel : BaseViewModel() {

    val listenner = object : OnItemClickListener<Video> {
        override fun onClick(view: View, item: Video) {
            val ptvPlayer = (view.parent as View).findViewById<PlayerTextureView>(R.id.ptv_player)
            check(ptvPlayer != null) { "not found PlayerTextureView" }

            when(view.id) {
                R.id.ivPlay -> {
                    if (!item.play) ptvPlayer.start(item.url) else ptvPlayer.stop()

                    item.play = !item.play
                    _itemBean.value = item

                    if (item.play)
                        refreshDuration(ptvPlayer)
                    else
                        isRefreshDurationExit = true
                }
                R.id.ivPaused -> {
                    ptvPlayer.tooglePause()
                    isRefreshDurationPaused = !isRefreshDurationPaused
                }
                R.id.ivMute -> {
                    ptvPlayer.toogleMute()
                }
            }
        }
    }

    val listenner2 = object : OnItemClickListener<String> {
        override fun onClick(view: View, item: String) {
            Log.d("native-lib", "onClick is $item")
            Bus.post("HIDDEN_RATE", item)
            setRate(item)
        }
    }

    private val _progress = MutableLiveData<Int>()
    val progress: LiveData<Int> = _progress

    private val _currentDuration = MutableLiveData<String>()
    val currentDuration: LiveData<String> = _currentDuration

    private val _itemBean = MutableLiveData<Video>()
    val itemBean: LiveData<Video> = _itemBean

    private val _rate = MutableLiveData<String>()
    val rate: LiveData<String> = _rate

    private val _items = MutableLiveData<MutableList<String>>()
    val items: LiveData<MutableList<String>> = _items

    val itemBinding = ItemBinding.of<String>(BR.itemBean, R.layout.fragment_rate_list_dialog_list_dialog_item)
        .bindExtra(BR.listenner, listenner2)

    var isRefreshDurationExit = false
    var isRefreshDurationPaused = false

    private val rates = listOf<String>("4X", "3X", "2X", "1.25x", "正常", "0.5X")
    private val ratev = listOf<Int>(2000, 1750, 1500, 1250, 1000, 500)


    fun initData(item : Video) {
        _itemBean.value = item
        _items.value = rates.toMutableList()
        _rate.value = rates[4]
    }

    fun setBean(item : Video) {
        _itemBean.value = item
    }

    fun updateDuration(paused: Boolean) {
        isRefreshDurationPaused = paused
    }

    fun refreshDuration(ptvPlayer: PlayerTextureView) {
        isRefreshDurationExit = false
        isRefreshDurationPaused = false

        launchUI {
            flow {
                while (!isRefreshDurationExit) {

                    if (isRefreshDurationPaused) {
                        delay(100)
                        continue
                    }

                    emit(ptvPlayer.getCurrentDuration())

                    delay(200)
                }
            }
            .flowOn(Dispatchers.IO)
            .collect {
                val duration = ptvPlayer.getDuration()
                var current_duration = it
                if (duration > 0 && current_duration > duration)
                    current_duration = duration

                if (duration.compareTo(0) != 0) {
                    val x = current_duration *  Int.MAX_VALUE
                    val y = x / duration
                    _progress.value = y.toInt()
                }
                _currentDuration.value = ptvPlayer.formatDuration(current_duration)

                if (current_duration > 0 && current_duration == duration)
                    isRefreshDurationExit = true
            }
        }
    }

    fun setRate(rate: String) {
        _rate.value = rate
        Bus.post("RATE", ratev[rates.indexOf(rate)])
    }
}