package com.kotlin.media.ui.main

import android.content.Intent
import android.util.Log
import android.view.View
import androidx.appcompat.app.AlertDialog
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.kotlin.media.R
import com.kotlin.media.model.bean.Video
import com.kotlin.mvvm.BR
import com.kotlin.mvvm.base.BaseViewModel
import com.kotlin.mvvm.base.OnItemClickListener
import me.tatarka.bindingcollectionadapter2.ItemBinding
import com.kotlin.media.data.local.VideoDao
import com.kotlin.media.ui.base.OnItemLongClickListener
import com.kotlin.media.ui.player.VideoPlayerActivity
import com.kotlin.media.ui.view.PlayerTextureView
import com.kotlin.mvvm.event.Message
import com.kotlin.mvvm.network.ExceptionHandle
import com.kotlin.mvvm.network.RESULT
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.flow.flowOn
import javax.inject.Inject

@HiltViewModel
class MainViewModel @Inject constructor() : BaseViewModel() {

    @Inject
    lateinit var videoDao: VideoDao


    private val onItemLongClickListener = object : OnItemLongClickListener<Video> {
        override fun onClick(view: View, item: Video) : Boolean {
            Log.d("EditViewModel", "MainViewModel.onLongClick:" + view)
            AlertDialog.Builder(view.context)
                .setMessage(R.string.confirm_delete)
                .setPositiveButton(R.string.confirm) { _, _ ->
                    deleteVideoList(item)
                }
                .setNegativeButton(R.string.cancel) { _, _ -> }
                .show()

            return true
        }

    }

    private val onItemClickListener = object : OnItemClickListener<Video> {
        override fun onClick(view: View, item: Video) {

            when(view.id) {
                R.id.ivPlay ->
                {
                    var ptvPlayer = (view.parent as View).findViewById<PlayerTextureView>(R.id.ptv_player)
                    ptvPlayer?.let {
                        if (!item.play) ptvPlayer.start(item.url) else ptvPlayer.stop()
                        item.play = !item.play
                    }

                    //_items.value = _items.value!!.toMutableList()

                    //if (item.play)
                    //    DeviceSurface.get().closeFfmpeg()
                    //else
                    //    DeviceSurface.get().openFfmpeg(item.url)
                }
                R.id.ivDetail ->
                {
                    view.context.startActivity(Intent().apply {
                        setClass(view.context, VideoPlayerActivity::class.java)
                        putExtra(VideoPlayerActivity.PARAM_VIDEO, item)
                    })
                }
                else ->
                {
                    Log.d("EditViewModel", "MainViewModel.onClick:" + view)
                }
            }
        }
    }

    private val _items = MutableLiveData<MutableList<Video>>()

    val items: LiveData<MutableList<Video>> = _items

    val itemBinding = ItemBinding.of<Video>(BR.itemBean, R.layout.item_video)
        .bindExtra(BR.listenner, onItemClickListener)
        .bindExtra(BR.listenner2, onItemLongClickListener)

    fun refreshVideoList() {
        /* _items.value = listOf<Video>(
           Video(1, "本地264", "/data/local/tmp/1080P.h264")
       ).toMutableList()

       _items.value = listOf<Video>(
           Video(1, "本地264", "/data/local/tmp/1080P.h264"),
           Video(2, "摄像机门口", "rtsp://admin:br123456789@192.168.1.39:554/avstream"),
           Video(3, "TCP流8206", "tcp://192.168.137.1:8206"),
           Video(4, "TCP流8207", "tcp://192.168.137.1:8207"),
           Video(5, "TCP流8208", "tcp://192.168.137.1:8208"),
           Video(6, "TCP流8209", "tcp://192.168.137.1:8209"),
       ).toMutableList()
     */
        Log.d("EditViewModel", "MainViewModel.videoDao:" + videoDao)

        launchUI {
            launchFlow {
                videoDao.getAllVideos()
            }
            .flowOn(Dispatchers.IO)
            .catch {
                val e = ExceptionHandle.handleException(it)
                callError(Message(e.code, e.msg))
            }
            .collect {
                _items.value = it
                callResult(RESULT.SUCCESS.code)
            }
        }
    }

    fun deleteVideoList(item: Video) {
        Log.d("EditViewModel", "MainViewModel.videoDao:" + videoDao)
        Log.d("EditViewModel", "MainViewModel.deleteVideoList:" + item)

        launchUI {
            launchFlow {
                videoDao.delete(item)
            }
            .flowOn(Dispatchers.IO)
            .catch {
                val e = ExceptionHandle.handleException(it)
                callError(Message(e.code, e.msg))
            }
            .collect {
                Log.d("EditViewModel", "MainViewModel.deleteVideoList.result:" + it)
                refreshVideoList()
                callResult(RESULT.SUCCESS.code)
            }
        }
    }

}