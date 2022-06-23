package com.kotlin.media.ui.group

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.kotlin.media.R
import com.kotlin.mvvm.BR
import com.kotlin.mvvm.base.BaseViewModel
import com.kotlin.mvvm.event.Message
import com.kotlin.mvvm.network.ExceptionHandle
import com.kotlin.mvvm.network.RESULT
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.flow.flowOn
import me.tang.bindingcollectionadapter.ItemBinding
import javax.inject.Inject

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

    private val _groups = MutableLiveData<MutableList<String>>()
    val groups: LiveData<MutableList<String>> = _groups
    val groupBinding = ItemBinding.of<String>(BR.itemBean, BR.groupPosition, R.layout.item_video_group)

    private val _childrens = MutableLiveData<MutableList<MutableList<String>>>()
    val childrens: LiveData<MutableList<MutableList<String>>> = _childrens
    val childBinding = ItemBinding.of<String>(BR.itemBean, BR.groupPosition, BR.childPosition, R.layout.item_video_children)

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