package com.kotlin.media.ui.edit

import android.util.Log
import androidx.lifecycle.MutableLiveData
import com.kotlin.media.R
import com.kotlin.media.data.local.VideoDao
import com.kotlin.mvvm.base.BaseViewModel
import dagger.hilt.android.lifecycle.HiltViewModel
import com.kotlin.media.model.bean.Video
import com.kotlin.mvvm.event.Message
import com.kotlin.mvvm.network.ExceptionHandle
import com.kotlin.mvvm.network.RESULT
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.*
import javax.inject.Inject

@HiltViewModel
class EditViewModel @Inject constructor() : BaseViewModel() {

    @Inject
    lateinit var videoDao: VideoDao

    val name = MutableLiveData<String>()
    val url = MutableLiveData<String>()

    val nameError = MutableLiveData<String>()
    val urlError = MutableLiveData<String>()

    fun click() {
        nameError.value = ""
        urlError.value = ""
        Log.d("EditViewModel", "name:" + name.value + ", url:" + url.value)
        when {
            name.value.isNullOrEmpty() -> nameError.value = getString(R.string.name_can_not_be_empty)
            url.value.isNullOrEmpty() -> urlError.value = getString(R.string.address_can_not_be_empty)
            else -> saveVideo()
        }
    }

    fun saveVideo() {

        Log.d("EditViewModel", "EditViewModel.videoDao:" + videoDao)

        launchUI {
            launchFlow {
                var v = Video(name = name.value!!, url = url.value!!)
                Log.d("EditViewModel", "name2:" + v.name + ", url:" + v.url)
                videoDao.insert(v)
                RESULT.END.code
            }
            .flowOn(Dispatchers.IO)
            .catch {
                val e = ExceptionHandle.handleException(it)
                callError(Message(e.code, e.msg))
            }
            .collect {
                callResult(it)
            }
        }
    }

    fun saveVideo2(videoDao2:VideoDao, v: Video) {
        launchUI {
            launchFlow {
                Log.d("EditViewModel", "name3:" + v.name + ", url:" + v.url)
                try {
                    videoDao2.insert(v)
                } catch (e:Exception) {
                    Log.d("EditViewModel", e.toString())
                }

                1
            }
                .flowOn(Dispatchers.IO)
                .collect {  }
        }
    }

}