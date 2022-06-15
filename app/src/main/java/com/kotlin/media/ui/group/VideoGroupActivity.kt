package com.kotlin.media.ui.group

import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.BaseExpandableListAdapter
import com.kotlin.media.databinding.ActivityVideoGroupBinding
import com.kotlin.media.databinding.ItemVideoChildrenBinding
import com.kotlin.media.databinding.ItemVideoGroupBinding
import com.kotlin.mvvm.base.BaseActivity
import com.kotlin.mvvm.base.NoViewModel

class VideoGroupActivity : BaseActivity<NoViewModel, ActivityVideoGroupBinding>() {

    val adapter by lazy { ExpandableListViewAdapter() }

    override fun initView(savedInstanceState: Bundle?) {
        mBinding.elvVideo.setOnChildClickListener { parent, v, groupPosition, childPosition, id ->

            val position = mBinding.elvVideo.pointToPosition(100, 100)
            var view = mBinding.elvVideo.getChildAt(position)
            Log.d("native-lib", "groupPosition:$groupPosition, childPosition:$childPosition, position:$position, view:$view")
            true
        }
    }

    override fun initData() {
        mBinding.elvVideo.setAdapter(adapter)
    }


    class ExpandableListViewAdapter : BaseExpandableListAdapter() {

        private val groups = listOf<String>("group1", "group2")
        private val childrens = listOf<List<String>>(listOf("children1", "children2", "children2", "children3", "children4", "children2", "children3", "children4", "children2", "children3", "children4"),
            listOf("children1", "children2", "children3", "children4", "children2", "children3", "children4", "children2", "children3", "children4", "children2", "children3", "children4", "children2", "children3", "children4"))

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

            binding.tvName.text = getChild(groupPosition, childPosition).toString()

            return binding.root
        }

        override fun isChildSelectable(groupPosition: Int, childPosition: Int): Boolean {
            return true
        }

    }
}