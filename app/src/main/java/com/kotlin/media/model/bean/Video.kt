package com.kotlin.media.model.bean

import android.os.Parcelable
import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.Ignore
import androidx.room.PrimaryKey
import kotlinx.parcelize.Parcelize

@Parcelize
@Entity(tableName = "videos")
data class Video (
    @PrimaryKey(autoGenerate = true)
    var id : Int = 0,
    var name: String,
    var url: String,
    var play: Boolean = false
) : Parcelable {}