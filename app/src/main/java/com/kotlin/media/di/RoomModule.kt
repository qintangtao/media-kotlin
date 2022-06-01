package com.kotlin.media.di

import android.app.Application
import android.util.Log
import androidx.room.Room
import com.kotlin.media.data.local.AppDataBase
import com.kotlin.media.data.local.VideoDao
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object RoomModule {

    @Provides
    @Singleton
    fun provideAppDataBase(application: Application): AppDataBase {
        return Room
            .databaseBuilder(application, AppDataBase::class.java, "media.db")
            .fallbackToDestructiveMigration()
            .allowMainThreadQueries()
            .build()
    }

    @Provides
    @Singleton
    fun providerVideoDao(appDataBase: AppDataBase): VideoDao {
        Log.d("EditViewModel", "videoDao:" + appDataBase.videoDao())
        return appDataBase.videoDao()
    }

}