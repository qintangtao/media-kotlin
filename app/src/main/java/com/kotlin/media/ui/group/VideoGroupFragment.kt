package com.kotlin.media.ui.group

import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AbsListView
import android.widget.BaseExpandableListAdapter
import androidx.databinding.DataBindingUtil
import androidx.databinding.ViewDataBinding
import androidx.fragment.app.Fragment
import com.kotlin.media.databinding.FragmentVideoGroupBinding
import com.kotlin.media.databinding.ItemVideoChildrenBinding
import com.kotlin.media.databinding.ItemVideoGroupBinding
import com.kotlin.mvvm.base.BaseFragment
import com.kotlin.mvvm.base.NoViewModel

// TODO: Rename parameter arguments, choose names that match
// the fragment initialization parameters, e.g. ARG_ITEM_NUMBER
private const val ARG_PARAM1 = "param1"
private const val ARG_PARAM2 = "param2"

/**
 * A simple [Fragment] subclass.
 * Use the [VideoGroupFragment.newInstance] factory method to
 * create an instance of this fragment.
 */
class VideoGroupFragment : BaseFragment<VideoGroupViewModel, FragmentVideoGroupBinding>() {
    // TODO: Rename and change types of parameters
    private var param1: String? = null
    private var param2: String? = null

    override fun initView(savedInstanceState: Bundle?) {
        super.initView(savedInstanceState)
        mBinding.viewModel = viewModel

        arguments?.let {
            param1 = it.getString(ARG_PARAM1)
            param2 = it.getString(ARG_PARAM2)
        }

        mBinding.elvVideo.setOnScrollListener(object : AbsListView.OnScrollListener {

            override fun onScrollStateChanged(view: AbsListView?, scrollState: Int) {}

            override fun onScroll(
                view: AbsListView?,
                firstVisibleItem: Int,
                visibleItemCount: Int,
                totalItemCount: Int
            ) {
                val firstVisibleView = mBinding.elvVideo.getChildAt(0)
                firstVisibleView?.let {
                    var _binding: ViewDataBinding? = DataBindingUtil.findBinding(firstVisibleView)
                    _binding?.let {
                        if (it is ItemVideoGroupBinding) {
                            val binding = it as ItemVideoGroupBinding
                            mBinding.tvFloatTitle.text = mBinding.elvVideo.expandableListAdapter.getGroup(binding.groupPosition).toString()
                        }

                        if (it is ItemVideoChildrenBinding) {
                            val binding = it as ItemVideoChildrenBinding
                            mBinding.tvFloatTitle.text = mBinding.elvVideo.expandableListAdapter.getGroup(binding.groupPosition).toString()
                        }
                    }
                }

                val nextVisibleView = mBinding.elvVideo.getChildAt(1)
                nextVisibleView?.let {
                    var _binding: ViewDataBinding? = DataBindingUtil.findBinding(nextVisibleView)

                    if (_binding is ItemVideoGroupBinding) {
                        mBinding.tvFloatTitle.y = if (it.top < mBinding.tvFloatTitle.measuredHeight) {
                            (it.top - mBinding.tvFloatTitle.measuredHeight).toFloat()
                        } else {
                            0f
                        }
                    }
                    if (_binding is ItemVideoChildrenBinding) {
                        mBinding.tvFloatTitle.y = 0f
                    }
                }
            }

        })
    }

    override fun lazyLoadData() {
        super.lazyLoadData()

        viewModel.refreshVideoList()
    }


    companion object {
        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @param param1 Parameter 1.
         * @param param2 Parameter 2.
         * @return A new instance of fragment VideoGroupFragment.
         */
        // TODO: Rename and change types and number of parameters
        @JvmStatic
        fun newInstance(param1: String, param2: String) =
            VideoGroupFragment().apply {
                arguments = Bundle().apply {
                    putString(ARG_PARAM1, param1)
                    putString(ARG_PARAM2, param2)
                }
            }
    }
}