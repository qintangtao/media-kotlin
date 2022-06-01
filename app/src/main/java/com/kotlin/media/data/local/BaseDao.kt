package com.kotlin.media.data.local

import androidx.room.*

@Dao
interface BaseDao<T> {

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun insertAll(list: MutableList<T>)

    @Delete
    fun delete(element: T): Int

    @Delete
    fun deleteList(elements:MutableList<T>)

    @Update
    fun update(element: T)

}