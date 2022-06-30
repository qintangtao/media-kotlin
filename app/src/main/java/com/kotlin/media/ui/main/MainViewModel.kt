package com.kotlin.media.ui.main

import android.content.Intent
import android.util.Log
import android.view.View
import androidx.appcompat.app.AlertDialog
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.recyclerview.widget.DiffUtil
import com.kotlin.media.R
import com.kotlin.media.model.bean.Video
import me.tatarka.bindingcollectionadapter2.ItemBinding
import com.kotlin.media.data.local.VideoDao
import com.kotlin.media.ui.base.OnItemLongClickListener
import com.kotlin.media.ui.player.VideoPlayerActivity
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.flow.flowOn
import me.tang.mvvm.base.BaseViewModel
import me.tang.mvvm.base.OnItemClickListener
import me.tang.mvvm.event.Message
import me.tang.mvvm.network.ExceptionHandle
import me.tang.mvvm.network.RESULT
import me.tang.mvvm.BR
import javax.inject.Inject
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.flow.onCompletion

@HiltViewModel
class MainViewModel @Inject constructor() : BaseViewModel() {

    @Inject
    lateinit var videoDao: VideoDao

    private val onItemLongClickListener = object : OnItemLongClickListener<Video> {
        override fun onClick(view: View, item: Video): Boolean {
            Log.d("EditViewModel", "MainViewModel.onLongClick:" + view)

            view.context.startActivity(Intent().apply {
                setClass(view.context, VideoPlayerActivity::class.java)
                putExtra(VideoPlayerActivity.PARAM_VIDEO, item)
            })

            return true

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

            when (view.id) {
                R.id.ivPlay -> {
                    var ptvPlayer =
                        (view.parent as View).findViewById<me.tang.videoplayerview.PlayerTextureView>(R.id.ptv_player)
                    ptvPlayer?.let {
                        if (!item.play) ptvPlayer.start(item.url) else ptvPlayer.stop()
                    }

                    val index = _items.value!!.indexOf(item)
                    //Log.d("EditViewModel", "item: $item, index: $index")
                    check(index > -1) { "not found $item from list" }

                    //Log.d("EditViewModel", "1 _items.value: ${_items.value}")

                    _items.value  = _items.value!!.toMutableList().apply {
                        removeAt(index)
                        add(index,  item.copy(play = !item.play))
                    }

                    //Log.d("EditViewModel", "4 _items.value: ${_items.value}")
                }
                //R.id.ivDetail -> {
                //    view.context.startActivity(Intent().apply {
                //        setClass(view.context, VideoPlayerActivity::class.java)
                //        putExtra(VideoPlayerActivity.PARAM_VIDEO, item)
                //    })
               // }
                else -> {
                    Log.d("EditViewModel", "MainViewModel.onClick:" + view)
                }
            }
        }
    }

    private val _items = MutableLiveData<MutableList<Video>>()
    val items: LiveData<MutableList<Video>> = _items

    val itemBinding = ItemBinding.of<Video>(BR.itemBean, R.layout.item_video_player)
        .bindExtra(BR.listenner, onItemClickListener)
        .bindExtra(BR.listenner2, onItemLongClickListener)

    val adapter = MyBindingRecyclerViewAdapter<Video>()

    val diff: DiffUtil.ItemCallback<Video> = object : DiffUtil.ItemCallback<Video>() {
        override fun areItemsTheSame(oldItem: Video, newItem: Video): Boolean {
            //Log.d("EditViewModel", "areItemsTheSame oldItem:$oldItem")
            //Log.d("EditViewModel", "areItemsTheSame newItem:$newItem")
            return oldItem.id == newItem.id
        }

        override fun areContentsTheSame(oldItem: Video, newItem: Video): Boolean {
            Log.d("EditViewModel", "areContentsTheSame oldItem:$oldItem")
            Log.d("EditViewModel", "areContentsTheSame newItem:$newItem")
            return oldItem.play == newItem.play
        }

    }

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
            .onCompletion {
                callComplete()
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