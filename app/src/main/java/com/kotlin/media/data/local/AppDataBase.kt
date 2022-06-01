package com.kotlin.media.data.local

import androidx.room.Database
import androidx.room.RoomDatabase
import com.kotlin.media.model.bean.Video

@Database(
    entities = [Video::class],
    version = 2, exportSchema = false
)
abstract class AppDataBase : RoomDatabase() {

    abstract fun videoDao() : VideoDao
}