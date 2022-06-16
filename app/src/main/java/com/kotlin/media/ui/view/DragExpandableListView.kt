package com.kotlin.media.ui.view

import android.content.Context
import android.util.AttributeSet
import android.widget.ExpandableListView

class DragExpandableListView : ExpandableListView {

    constructor(context: Context): this(context, null)
    constructor(context: Context, attrs: AttributeSet?): this(context, attrs, 0x0101006f)
    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int): this(context, attrs, defStyleAttr, 0)
    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int, defStyleRes: Int): super(context, attrs, defStyleAttr, defStyleRes) {

    }
}