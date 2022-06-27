package com.kotlin.media.ui.player

import android.os.Bundle
import android.util.Log
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.FragmentManager
import com.blankj.utilcode.util.ScreenUtils
import com.google.android.material.bottomsheet.BottomSheetBehavior
import com.google.android.material.bottomsheet.BottomSheetDialog
import com.kotlin.media.databinding.FragmentRateListDialogListDialogBinding
import com.kotlin.mvvm.base.BaseBottomSheetDialogFragment
import com.kotlin.mvvm.bus.Bus

/**
 *
 * A fragment that shows a list of items as a modal bottom sheet.
 *
 * You can show this modal bottom sheet from your activity like this:
 * <pre>
 *    RateListDialogFragment.newInstance(30).show(supportFragmentManager, "dialog")
 * </pre>
 */
class RateListDialogFragment: BaseBottomSheetDialogFragment<VideoPlayerViewModel, FragmentRateListDialogListDialogBinding>() {


    private var _height: Int? = null
    private val height get() = _height!!

    private var _behavior: BottomSheetBehavior<View>? = null
    private val behavior get() = _behavior!!

    companion object {
        fun newInstance() : RateListDialogFragment =  RateListDialogFragment()
    }

    override fun isShareVM(): Boolean = true

    override fun initView(savedInstanceState: Bundle?) {
        super.initView(savedInstanceState)

        Log.d("native-lib", "mBinding is $mBinding, viewModel is $viewModel")
        mBinding.viewModel = viewModel

        Bus.observe<String>("HIDDEN_RATE", this) {
            behavior.state = BottomSheetBehavior.STATE_HIDDEN
        }
    }

    override fun onStart() {
        super.onStart()
        Log.d("native-lib", "onStart")
        val bottomSheet: View = (dialog as BottomSheetDialog).delegate
            .findViewById(com.google.android.material.R.id.design_bottom_sheet)
            ?: return
        _behavior = BottomSheetBehavior.from(bottomSheet)
        Log.d("native-lib", "bottomSheet is $bottomSheet, height is $height,  behavior is $behavior")
        behavior.state = BottomSheetBehavior.STATE_EXPANDED

        return
        behavior.peekHeight = height
        dialog?.window?.let {
            Log.d("native-lib", "window is $it")
            it.setGravity(Gravity.BOTTOM)
            it.setLayout(
                //ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT
                ViewGroup.LayoutParams.MATCH_PARENT, height
            )
        }
    }

    fun show(manager: FragmentManager, height: Int? = null) {
        _height = height ?: (ScreenUtils.getScreenHeight() * 0.75f).toInt()
        super.show(manager, RateListDialogFragment::class.java.name)
    }

    override fun onDestroyView() {
        super.onDestroyView()
    }
}