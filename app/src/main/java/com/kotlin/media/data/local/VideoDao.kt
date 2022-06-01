package com.kotlin.media.data.local

import com.kotlin.media.model.bean.Video
import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query

@Dao
interface VideoDao: BaseDao<Video> {

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(element: Video)

    @Query("select * from videos")
    suspend fun getAllVideos():MutableList<Video>

    @Query("select * from videos where id = :id")
    suspend fun getVideo(id:Int):Video

    @Query("delete from videos")
    suspend fun deleteAll()
}