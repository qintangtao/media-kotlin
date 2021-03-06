package me.tang.videoplayerview

import android.content.Context
import android.content.res.TypedArray
import android.util.AttributeSet
import android.util.Log
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.constraintlayout.widget.ConstraintLayout
import com.blankj.utilcode.util.SizeUtils
import kotlinx.coroutines.*
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.*

class VideoPlayerView : ConstraintLayout {

    private val radius: Float = 12.0F

    private var _url: String? = null
    private val url get() = _url!!

    private lateinit var playerTextureView: PlayerTextureView
    private lateinit var foregroundView: View
    private lateinit var nameTextView: TextView
    private lateinit var playImageView: ImageView

    private val mainScope = MainScope()

    private var floatVisibility = View.VISIBLE


    constructor(context: Context) : this(context, null)
    constructor(context: Context, attrs: AttributeSet?) : this(context, attrs, 0)
    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : this(
        context,
        attrs,
        defStyleAttr,
        0
    )

    constructor(
        context: Context,
        attrs: AttributeSet?,
        defStyleAttr: Int,
        defStyleRes: Int
    ) : super(context, attrs, defStyleAttr, defStyleRes) {
        init(context, attrs, defStyleAttr, defStyleRes)
    }

    fun setText(text: String) {
        nameTextView.setText(text)
    }

    fun setUrl(url: String) {
        _url = url
    }

    fun setForegroundVisibility(visibility: Int) {
        foregroundView.visibility = visibility
    }

    fun setPlayVisibility(visibility: Int) {
        playImageView.visibility = visibility
    }

    private fun init(context: Context, attrs: AttributeSet?, defStyleAttr: Int, defStyleRes: Int) {
        val a: TypedArray = context.obtainStyledAttributes(attrs, R.styleable.VideoPlayerView)

        playerTextureView = PlayerTextureView(context).apply {
            layoutParams = ConstraintLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            ).apply {
                leftToLeft = LayoutParams.PARENT_ID
                rightToRight = LayoutParams.PARENT_ID
                topToTop = LayoutParams.PARENT_ID
                bottomToBottom = LayoutParams.PARENT_ID
                circleRadius = SizeUtils.dp2px(radius)
            }
        }
        addView(playerTextureView)

        foregroundView = View(context).apply {
            layoutParams = ConstraintLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            ).apply {
                leftToLeft = LayoutParams.PARENT_ID
                rightToRight = LayoutParams.PARENT_ID
                topToTop = LayoutParams.PARENT_ID
                bottomToBottom = LayoutParams.PARENT_ID
                //circleRadius = SizeUtils.dp2px(radius)
                alpha = 0.8f
                background =
                    a.getDrawable(R.styleable.VideoPlayerView_foreground_background)// ?: a.getDrawable(R.drawable.shape_bg_video)
            }
        }
        addView(foregroundView)

        Log.d(
            "EditViewModel",
            "id:${R.style.VideoPlayerView_text}, newID:${
                a.getResourceId(
                    R.styleable.VideoPlayerView_text_appearance,
                    0
                )
            }"
        )

        nameTextView = TextView(
            context,
            null,
            0,
            a.getResourceId(R.styleable.VideoPlayerView_text_appearance, 0)
        ).apply {
            layoutParams = ConstraintLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply {
                leftToLeft = LayoutParams.PARENT_ID
                topToTop = LayoutParams.PARENT_ID
                //leftMargin = SizeUtils.dp2px(9.0f).toInt()
                topMargin = SizeUtils.dp2px(6.0f).toInt()
                marginStart = SizeUtils.dp2px(9.0f).toInt()
                //marginTop= SizeUtils.dp2px(6.0f).toInt()
            }

            text = a.getString(R.styleable.VideoPlayerView_text)
        }
        addView(nameTextView)

        playImageView = ImageView(context).apply {
            layoutParams = ConstraintLayout.LayoutParams(
                SizeUtils.dp2px(48.0f),
                SizeUtils.dp2px(48.0f)
            ).apply {
                leftToLeft = LayoutParams.PARENT_ID
                rightToRight = LayoutParams.PARENT_ID
                topToTop = LayoutParams.PARENT_ID
                bottomToBottom = LayoutParams.PARENT_ID
            }
            background = a.getDrawable(R.styleable.VideoPlayerView_play_background)
            setOnClickListener {
                if (playerTextureView.isPlaying()) {
                    playerTextureView.stop()
                    setForegroundVisibility(View.VISIBLE)
                    setFloatVisibility(View.VISIBLE)
                } else {
                    playerTextureView.start(url)
                    setForegroundVisibility(View.GONE)
                    setFloatVisibility(View.GONE)
                }
            }
        }
        addView(playImageView)

        a.recycle()

        clickFlow()
            .filter {
                Log.d("EditViewModel", "click 1")
                if (playerTextureView.isPlaying()) {
                    if (playImageView.visibility == View.GONE) {
                        setFloatVisibility(View.VISIBLE)
                        return@filter true
                    }
                    setFloatVisibility(View.GONE)
                }
                return@filter false
            }
            .debounce(3000)
            .onEach {
                Log.d("EditViewModel", "click 2")
                if (playerTextureView.isPlaying())
                    setFloatVisibility(View.GONE)
            }
            .launchIn(mainScope)

        /*
        <me.tang.videoplayerview.PlayerTextureView
        android:id="@+id/ptv_player"
        android:layout_width="match_parent"
        android:layout_height="match_parent"
        android:radius="12dp"
        app:layout_constraintLeft_toLeftOf="parent"
        app:layout_constraintRight_toRightOf="parent"
        app:layout_constraintTop_toTopOf="parent"
        app:layout_constraintBottom_toBottomOf="parent"
        />

    <View
        android:layout_width="match_parent"
        android:layout_height="match_parent"
        android:background="@drawable/shape_bg_video"
        android:alpha="0.8"
        android:visibility="@{itemBean.play ? View.GONE : View.VISIBLE}"
        app:layout_constraintLeft_toLeftOf="parent"
        app:layout_constraintRight_toRightOf="parent"
        app:layout_constraintTop_toTopOf="parent"
        app:layout_constraintBottom_toBottomOf="parent"
        />

    <TextView
        android:id="@+id/tvName"
        android:layout_width="wrap_content"
        android:layout_height="wrap_content"
        android:text="@{itemBean.name}"
        app:layout_constraintLeft_toLeftOf="parent"
        app:layout_constraintTop_toTopOf="parent"
        android:layout_marginStart="9dp"
        android:layout_marginTop="6dp"
        android:textColor="@color/white"
        />

    <ImageView
        android:id="@+id/ivPlay"
        android:layout_width="48dp"
        android:layout_height="48dp"
        android:src="@drawable/ic_chevron_right_black_24dp"
        android:onClick="@{(view)->listenner.onClick(view, itemBean)}"
        app:layout_constraintLeft_toLeftOf="parent"
        app:layout_constraintRight_toRightOf="parent"
        app:layout_constraintTop_toTopOf="parent"
        app:layout_constraintBottom_toBottomOf="parent" />

    <ImageView
        android:id="@+id/ivDetail"
        android:layout_width="24dp"
        android:layout_height="24dp"
        android:layout_marginEnd="9dp"
        android:layout_marginTop="6dp"
        android:src="@drawable/ic_chevron_right_black_24dp"
        android:onClick="@{(view)->listenner.onClick(view, itemBean)}"
        app:layout_constraintRight_toRightOf="parent"
        app:layout_constraintTop_toTopOf="parent" />
        * */
    }

    private fun setFloatVisibility(visibility: Int) {
        if (floatVisibility == visibility)
            return

        floatVisibility = visibility

        playImageView.visibility = visibility
        nameTextView.visibility = visibility
    }
}

fun View.clickFlow() = callbackFlow {
    setOnClickListener { offer(Unit) }
    awaitClose { setOnClickListener(null) }
}

fun <T> Flow<T>.throttleFirst(thresholdMillis: Long): Flow<T> = flow {
    var lastTime = 0L // ???????????????????????????
    // ????????????
    collect { upstream ->
        // ????????????
        val currentTime = System.currentTimeMillis()
        // ???????????????????????????????????????????????????
        if (currentTime - lastTime > thresholdMillis) {
            lastTime = currentTime
            emit(upstream)
        }
    }
}