package com.kotlin.media.ui.group

import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AbsListView
import android.widget.BaseExpandableListAdapter
import androidx.recyclerview.widget.LinearLayoutManager
import com.kotlin.media.databinding.ActivityVideoGroupBinding
import com.kotlin.media.databinding.ItemVideoChildrenBinding
import com.kotlin.media.databinding.ItemVideoGroupBinding
import com.kotlin.mvvm.base.BaseActivity
import com.kotlin.mvvm.base.NoViewModel

class VideoGroupActivity : BaseActivity<NoViewModel, ActivityVideoGroupBinding>() {
    private val groups = listOf<String>("group1", "group2", "group3", "group4")
    private val childrens = listOf<List<String>>(
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

    private val adapter by lazy { ExpandableListViewAdapter(groups, childrens) }

    private var currentPosition = 0
    private var currentY = 0f

    override fun initView(savedInstanceState: Bundle?) {
        mBinding.elvVideo.setOnChildClickListener { parent, v, groupPosition, childPosition, id ->

            val position = mBinding.elvVideo.pointToPosition(100, 100)
            var view = mBinding.elvVideo.getChildAt(position)
            Log.d(
                "native-lib",
                "groupPosition:$groupPosition, childPosition:$childPosition, position:$position, view:$view"
            )
            true
        }

        mBinding.tvFloatTitle.text = groups[currentPosition]

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
                    if (it.tag is ItemVideoGroupBinding) {
                        val binding = it.tag as ItemVideoGroupBinding
                        //Log.d("native-lib", "firstVisibleView ItemVideoGroupBinding tag:${binding.tvName.tag as Int}")
                        mBinding.tvFloatTitle.text = adapter.getGroup(binding.tvName.tag as Int)!!.toString()
                    }

                    if (it.tag is ItemVideoChildrenBinding) {
                        val binding = it.tag as ItemVideoChildrenBinding
                        //Log.d("native-lib", "firstVisibleView ItemVideoChildrenBinding tag:${binding.tvName.tag as Int}")
                        mBinding.tvFloatTitle.text = adapter.getGroup(binding.tvName.tag as Int)!!.toString()
                    }
                }

                val nextVisibleView = mBinding.elvVideo.getChildAt(1)
                nextVisibleView?.let {
                    if (it.tag is ItemVideoGroupBinding) {
                        val binding = it.tag as ItemVideoGroupBinding
                        //Log.d("native-lib", "nextVisibleView ItemVideoGroupBinding text:${binding.tvName.text}， top:${it.top}")
                        mBinding.tvFloatTitle.y = if (it.top < mBinding.tvFloatTitle.measuredHeight) {
                            (it.top - mBinding.tvFloatTitle.measuredHeight).toFloat()
                        } else {
                            0f
                        }
                    }
                    if (it.tag is ItemVideoChildrenBinding) {
                        mBinding.tvFloatTitle.y = 0f
                    }
                }
            }

        })
    }

    override fun initData() {
        mBinding.elvVideo.setAdapter(adapter)
    }


    class ExpandableListViewAdapter(
        val groups: List<String>,
        val childrens: List<List<String>>
    ) : BaseExpandableListAdapter() {

        override fun getGroupCount(): Int {
            return groups.size
        }

        override fun getChildrenCount(groupPosition: Int): Int {
            return childrens[groupPosition].size
        }

        override fun getGroup(groupPosition: Int): Any {
            return groups[groupPosition]
        }

        override fun getChild(groupPosition: Int, childPosition: Int): Any {
            return childrens[groupPosition][childPosition]
        }

        override fun getGroupId(groupPosition: Int): Long {
            return groupPosition.toLong()
        }

        override fun getChildId(groupPosition: Int, childPosition: Int): Long {
            return childPosition.toLong()
        }

        override fun hasStableIds(): Boolean {
            return true
        }

        override fun getGroupView(
            groupPosition: Int,
            isExpanded: Boolean,
            convertView: View?,
            parent: ViewGroup?
        ): View {
            var binding: ItemVideoGroupBinding
            if (convertView == null) {
                binding = ItemVideoGroupBinding.inflate(LayoutInflater.from(parent!!.context))
                binding.root.setTag(binding)
            } else {
                binding = convertView.tag as ItemVideoGroupBinding
            }
            binding.tvName.setTag(groupPosition)
            binding.tvName.text = getGroup(groupPosition).toString()

            return binding.root
        }

        override fun getChildView(
            groupPosition: Int,
            childPosition: Int,
            isLastChild: Boolean,
            convertView: View?,
            parent: ViewGroup?
        ): View {
            var binding: ItemVideoChildrenBinding
            if (convertView == null) {
                binding = ItemVideoChildrenBinding.inflate(LayoutInflater.from(parent!!.context))
                binding.root.setTag(binding)
            } else {
                binding = convertView.tag as ItemVideoChildrenBinding
            }

            binding.tvName.setTag(groupPosition)
            binding.tvName.text = getChild(groupPosition, childPosition).toString()

            return binding.root
        }

        override fun isChildSelectable(groupPosition: Int, childPosition: Int): Boolean {
            return true
        }

    }
}