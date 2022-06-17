package com.kotlin.media.ui.view

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Bitmap
import android.graphics.PixelFormat
import android.os.Handler
import android.os.Looper
import android.util.AttributeSet
import android.util.Log
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.WindowManager
import android.widget.ExpandableListView
import android.widget.ImageView


class DragExpandableListView : ExpandableListView {

    companion object {
        const val TAG = "DragExpandableListView"
    }

    private val handler2 by lazy { Handler(Looper.getMainLooper()) }

    private val runnable2 by lazy { object : Runnable {
        override fun run() {
            Log.d(TAG, "runnable2: run:")
            isDrag = true
            mDragBitmap?.let {
                createDragImage(it, mDownX, mDownY)
            }
        }
    } }

    private var mWindowManager: WindowManager? = null
    private val mWindowLayoutParams by lazy { WindowManager.LayoutParams() }

    private var isDrag = false

    private var mDragBitmap: Bitmap? = null
    private var mDragImageView: ImageView? = null

    private var lastDownTime: Long = 0
    private var currDownTime: Long = 0

    private  var mDownX: Float = 0f
    private  var mDownY: Float = 0f

    constructor(context: Context): this(context, null)
    constructor(context: Context, attrs: AttributeSet?): this(context, attrs, 0x0101006f)
    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int): this(context, attrs, defStyleAttr, 0)
    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int, defStyleRes: Int): super(context, attrs, defStyleAttr, defStyleRes) {

        mWindowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager

    }

    override fun onTouchEvent(ev: MotionEvent?): Boolean {

        ev?.let {
            Log.d(TAG, "onTouchEvent: action: ${it.action}")

            when(it.action) {
                MotionEvent.ACTION_DOWN -> {
                    mDownX = ev.x
                    mDownY = ev.y
                    val position =  pointToPosition(ev.x.toInt(), ev.y.toInt())
                    if (position != INVALID_POSITION) {
                        val view = getChildAt(position - firstVisiblePosition)
                        view?.let {
                            lastDownTime = currDownTime
                            currDownTime = System.currentTimeMillis()
                            if (currDownTime-lastDownTime >= 300)
                            {
                                view.setDrawingCacheEnabled(true);
                                mDragBitmap = Bitmap.createBitmap(view.drawingCache)
                                view.destroyDrawingCache()
                                handler2.postDelayed(runnable2, 300)
                            }
                            else
                            {
                                this.currDownTime = 0;
                                this.lastDownTime = 0;
                            }
                        }
                    }
                }
                MotionEvent.ACTION_CANCEL,
                MotionEvent.ACTION_POINTER_UP -> {
                    handler2.removeCallbacks(runnable2)
                }
                MotionEvent.ACTION_UP -> {
                    //Log.d(TAG, "onTouchEvent: action: ACTION_UP")
                    handler2.removeCallbacks(runnable2)
                    if (isDrag)
                    {
                        removeDragImage()
                        isDrag = false
                    }
                }
                MotionEvent.ACTION_MOVE -> {
                    //Log.d(TAG, "onTouchEvent: action: ACTION_MOVE")
                    if (isDrag)
                    {
                        onDragItem(it.x, it.y)
                    }
                    return false
                }
                else -> {}
            }
        }

        return super.onTouchEvent(ev)
    }


    private fun onDragItem(x: Float, y: Float) {
        Log.d(TAG, "onDragItem: x:$x, y:$y")
        val locationOnScreen = getLocationOnScreen(this)
        mWindowLayoutParams.x = getLocationWidth(x.toInt(), locationOnScreen!![0])
        mWindowLayoutParams.y = getLocationHeight(y.toInt(), locationOnScreen!![1])
        mWindowManager?.run {
            updateViewLayout(mDragImageView, mWindowLayoutParams)
        }
    }

    private fun createDragImage(bitmap: Bitmap, x: Float, y: Float) {
        Log.d(TAG, "createDragImage: x:$x, y:$y")
        val locationOnScreen = getLocationOnScreen(this)
        mWindowLayoutParams.format = PixelFormat.TRANSLUCENT //支持半透明的格式
        mWindowLayoutParams.gravity = Gravity.TOP or Gravity.LEFT
        mWindowLayoutParams.x = getLocationWidth(x.toInt(), locationOnScreen!![0])
        mWindowLayoutParams.y = getLocationHeight(y.toInt(), locationOnScreen!![1])
        mWindowLayoutParams.alpha = 80f
        mWindowLayoutParams.width = bitmap.width
        mWindowLayoutParams.height = bitmap.height
        mWindowLayoutParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
                WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE or
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
        mDragImageView = ImageView(context).apply {
            setImageBitmap(bitmap)
        }
        mWindowManager?.run {
            addView(mDragImageView, mWindowLayoutParams)
        }
    }

    private fun removeDragImage() {
        mWindowManager?.let {
            mDragImageView?.let {
                mWindowManager!!.removeView(it)
                mDragImageView = null
            }
        }
    }

    private fun getLocationWidth(x: Int, locationX: Int): Int {
        return ((locationX + x) - 40*0.8).toInt()
    }
    private fun getLocationHeight(y: Int, locationY: Int): Int {
        return ((locationY + y) - 160 - 25*0.8).toInt()
    }

    private fun getLocationOnScreen(view: View): IntArray? {
        val iArr = IntArray(2)
        view.getLocationOnScreen(iArr)
        return iArr
    }

}