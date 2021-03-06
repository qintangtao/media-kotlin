package com.kotlin.media.ui.group

import android.content.Intent
import android.view.View
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.blankj.utilcode.util.ActivityUtils
import com.blankj.utilcode.util.ScreenUtils
import com.kotlin.media.R
import com.kotlin.media.model.bean.Video
import com.kotlin.media.ui.player.VideoPlayerActivity
import me.tang.mvvm.BR
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.flow.flowOn
import me.tang.bindingcollectionadapter.ItemBinding
import me.tang.mvvm.base.BaseViewModel
import me.tang.mvvm.event.Message
import me.tang.mvvm.network.ExceptionHandle
import me.tang.mvvm.network.RESULT
import javax.inject.Inject
import kotlinx.coroutines.flow.collect
import me.tang.mvvm.base.OnItemClickListener

@HiltViewModel
class VideoGroupViewModel @Inject constructor() : BaseViewModel() {

    private val __groups = listOf("group1", "group2", "group3", "group4")
    private val __childrens = listOf(
        listOf(
            "children1",
            "children2",
            "children3",
            "children4",
            "children5",
            "children6",
            "children7",
            "children8",
            "children9",
            "children10",
            "children11"
        ),
        listOf(
            "children1",
            "children2",
            "children3",
            "children4",
            "children5",
            "children6",
            "children7",
            "children8",
            "children9",
            "children10",
            "children11"
        ),
        listOf(
            "children1",
            "children2",
            "children3",
            "children4",
            "children5",
            "children6",
            "children7",
            "children8",
            "children9",
            "children10",
            "children11"
        ),
        listOf(
            "children1",
            "children2",
            "children3",
            "children4",
            "children5",
            "children6",
            "children7",
            "children8",
            "children9",
            "children10",
            "children11"
        )
    )

    val listener = object : OnItemClickListener<String> {
        override fun onClick(view: View, item: String) {
            val v = Video(1, "v1080.mp4", "/data/local/tmp/v1080.mp4")
            if (ScreenUtils.isPortrait())
            {
                view.context.startActivity(Intent().apply {
                    setClass(view.context, VideoPlayerActivity::class.java)
                    putExtra(VideoPlayerActivity.PARAM_VIDEO, v)
                })
            }

        }
    }

    private val _groups = MutableLiveData<MutableList<String>>()
    val groups: LiveData<MutableList<String>> = _groups
    val groupBinding = ItemBinding.of<String>(BR.itemBean, BR.groupPosition, R.layout.item_video_group)

    private val _childrens = MutableLiveData<MutableList<MutableList<String>>>()
    val childrens: LiveData<MutableList<MutableList<String>>> = _childrens
    val childBinding = ItemBinding.of<String>(BR.itemBean, BR.groupPosition, BR.childPosition, R.layout.item_video_children)
        .bindExtra(BR.listenner, listener)


    fun refreshVideoList() {

        launchUI {
            launchFlow {
                __groups.toMutableList()
            }
            .flowOn(Dispatchers.IO)
            .catch {
                val e = ExceptionHandle.handleException(it)
                callError(Message(e.code, e.msg))
            }
                .collect {
                    _groups.value = it
                    callResult(RESULT.SUCCESS.code)
                }
        }

        launchUI {
            launchFlow {
                val childrenss = emptyList<MutableList<String>>().toMutableList()
                __childrens.forEach {
                    childrenss.add(it.toMutableList())
                }
                childrenss
            }
            .flowOn(Dispatchers.IO)
            .catch {
                val e = ExceptionHandle.handleException(it)
                callError(Message(e.code, e.msg))
            }
            .collect {
                _childrens.value = it
                callResult(RESULT.SUCCESS.code)
            }
        }
    }

}