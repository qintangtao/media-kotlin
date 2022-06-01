package com.kotlin.media.ui.base

import android.view.View

interface OnItemLongClickListener<T> {
    fun onClick(view: View, item: T) : Boolean
}