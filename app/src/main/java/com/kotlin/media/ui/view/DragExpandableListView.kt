package com.kotlin.media.ui.view

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
import com.blankj.utilcode.util.BarUtils
import com.blankj.utilcode.util.SizeUtils


class DragExpandableListView : ExpandableListView {

    companion object {
        const val TAG = "DragExpandableListView"
    }

    private val handler2 by lazy { Handler(Looper.getMainLooper()) }

    private val runnable2 by lazy { object : Runnable {
        override fun run() {
            Log.d(TAG, "runnable2: run:")
            //parent.requestDisallowInterceptTouchEvent(true)
            mDragBitmap?.let {
                createDragImage(it, mDownX, mDownY)
            }
            isDrag = true
        }
    } }

    private var mWindowManager: WindowManager? = null
    private val mWindowLayoutParams by lazy { WindowManager.LayoutParams() }

    private var isDrag = false

    private var mDragBitmap: Bitmap? = null
    private var mDragImageView: ImageView? = null

    private val dragImageHeight by lazy { SizeUtils.dp2px(40.0f) }
    private val dragImageWidth by lazy { SizeUtils.dp2px(100.0f) }


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

    override fun dispatchTouchEvent(ev: MotionEvent?): Boolean {
        ev?.let {
            when(it.action) {
                MotionEvent.ACTION_DOWN -> {
                    lastDownTime = System.currentTimeMillis()
                    parent.requestDisallowInterceptTouchEvent(true)
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
                        parent.requestDisallowInterceptTouchEvent(false)
                    }
                }
                MotionEvent.ACTION_MOVE -> {
                    //Log.d(TAG, "onTouchEvent: action: ACTION_MOVE")
                    if (!isDrag)
                    {
                        currDownTime = System.currentTimeMillis()
                        if (lastDownTime > 0)
                            Log.d(TAG, "ACTION_DOWN: lastDownTime: $lastDownTime, currDownTime: $currDownTime   diff: ${currDownTime-lastDownTime}")
                        if (lastDownTime > 0 && currDownTime-lastDownTime >= 200)
                        {
                            mDownX = ev.x
                            mDownY = ev.y
                            val position =  pointToPosition(ev.x.toInt(), ev.y.toInt())
                            if (position != INVALID_POSITION) {
                                val view = getChildAt(position - firstVisiblePosition)
                                view?.let {
                                    it.setDrawingCacheEnabled(true);
                                    mDragBitmap = Bitmap.createBitmap(it.drawingCache)
                                    it.destroyDrawingCache()
                                    handler2.postDelayed(runnable2, 200)
                                }
                            }
                        }
                        else
                        {
                            parent.requestDisallowInterceptTouchEvent(false)
                        }

                        this.currDownTime = 0;
                        this.lastDownTime = 0;
                    }
                }
                else -> {}
            }

        }


        return super.dispatchTouchEvent(ev)
    }

    override fun onTouchEvent(ev: MotionEvent?): Boolean {
        ev?.let {
            when(it.action) {
                MotionEvent.ACTION_MOVE -> {
                    if (isDrag)
                    {
                        onDragItem(it.x, it.y)
                        return false
                    }
                }
            }
        }
        return super.onTouchEvent(ev)
    }

    /*
       override fun onTouchEvent(ev: MotionEvent?): Boolean {

           ev?.let {
               //Log.d(TAG, "onTouchEvent: action: ${it.action}")

               when(it.action) {
                   MotionEvent.ACTION_DOWN -> {
                       lastDownTime = System.currentTimeMillis()
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
                           parent.requestDisallowInterceptTouchEvent(false)
                       }
                   }
                   MotionEvent.ACTION_MOVE -> {
                       //Log.d(TAG, "onTouchEvent: action: ACTION_MOVE")
                       if (isDrag)
                       {
                           onDragItem(it.x, it.y)
                           return false
                       }
                       else
                       {
                           currDownTime = System.currentTimeMillis()
                           if (lastDownTime > 0)
                               Log.d(TAG, "ACTION_DOWN: lastDownTime: $lastDownTime, currDownTime: $currDownTime   diff: ${currDownTime-lastDownTime}")
                           if (lastDownTime > 0 && currDownTime-lastDownTime >= 200)
                           {
                               mDownX = ev.x
                               mDownY = ev.y
                               val position =  pointToPosition(ev.x.toInt(), ev.y.toInt())
                               if (position != INVALID_POSITION) {
                                   val view = getChildAt(position - firstVisiblePosition)
                                   view?.let {
                                       it.setDrawingCacheEnabled(true);
                                       mDragBitmap = Bitmap.createBitmap(it.drawingCache)
                                       it.destroyDrawingCache()
                                       handler2.postDelayed(runnable2, 200)
                                   }
                               }
                           }

                           this.currDownTime = 0;
                           this.lastDownTime = 0;
                       }
                   }
                   else -> {}
               }
           }

           return super.onTouchEvent(ev)
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
                               Log.d(TAG, "ACTION_DOWN: lastDownTime: $lastDownTime, currDownTime: $currDownTime")
                               if (lastDownTime > 0 && currDownTime-lastDownTime >= 300)
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
                           parent.requestDisallowInterceptTouchEvent(false)
                       }
                   }
                   MotionEvent.ACTION_MOVE -> {
                       //Log.d(TAG, "onTouchEvent: action: ACTION_MOVE")
                       if (isDrag)
                       {
                           onDragItem(it.x, it.y)
                           return false
                       }
                   }
                   else -> {}
               }
           }

           return super.onTouchEvent(ev)
       }
   */
    private fun onDragItem(x: Float, y: Float) {
        //Log.d(TAG, "onDragItem: x:$x, y:$y")
        mWindowLayoutParams.x = getLocationX(x.toInt())
        mWindowLayoutParams.y = getLocationY(y.toInt())
        mWindowManager?.run {
            updateViewLayout(mDragImageView, mWindowLayoutParams)
        }
    }

    private fun createDragImage(bitmap: Bitmap, x: Float, y: Float) {
        Log.d(TAG, "createDragImage: x:$x, y:$y, bitmap.width:${bitmap.width}, bitmap.height:${bitmap.height}")
        mWindowLayoutParams.format = PixelFormat.TRANSLUCENT //支持半透明的格式
        mWindowLayoutParams.gravity = Gravity.TOP or Gravity.LEFT
        mWindowLayoutParams.x = getLocationX(x.toInt())
        mWindowLayoutParams.y = getLocationY(y.toInt())
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

    private fun getLocationX(x: Int): Int {
        //return ((locationX + x) - dragImageWidth*0.8).toInt()
        return (getViewX(this) + x - dragImageWidth*0.8).toInt()
    }
    private fun getLocationY(y: Int): Int {
        //return ((locationY + y) - BarUtils.getStatusBarHeight() - dragImageHeight*0.8).toInt()
        return (getViewY(this) + y - BarUtils.getStatusBarHeight()).toInt()
    }

    private fun getViewX(view: View): Int {
        val point = IntArray(2)
        view.getLocationOnScreen(point)
        return point[0]
    }

    private fun getViewY(view: View): Int {
        val point = IntArray(2)
        view.getLocationOnScreen(point)
        return point[1]
    }

}