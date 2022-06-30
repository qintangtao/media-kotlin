# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

#set(ProjectRoot E:/Qt/Qt5.12.2/Licenses/open/NDKDemo01)
#set(LIBSDir ${ProjectRoot}/app/src/main/cpp)


#include_directories(${CMAKE_SOURCE_DIR}/include)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
            ffplay

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             native-lib.cpp
             AMediaDecoder.cpp
             TCPSocket.cpp
             YuvData.cpp
        RGBData.cpp
        WindowData.cpp
        QueueWindowData.cpp
        ffplay.cpp
        looper.cpp
        common.cpp
        opensl_io.c
        yuv_util.cpp
        time_util.cpp)


#设置要连接的so的相对路径，${ANDROID_ABI}表示so文件的ABI类型的路径,这一步引入了动态加入编译的so
include_directories(libyuv/include)
add_library(libyuv
        SHARED
        IMPORTED)
set_target_properties(libyuv
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libyuv/libs/${ANDROID_ABI}/libyuv.so)


include_directories(libffmpeg/include)
#[[
add_library(libffmpeg
        SHARED
        IMPORTED)
set_target_properties(libffmpeg
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libffmpeg/libs/${ANDROID_ABI}/libffmpeg.so)
]]

add_library(libavutil
        SHARED
        IMPORTED)
set_target_properties(libavutil
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libffmpeg/libs/${ANDROID_ABI}/libavutil.so)

add_library(libavcodec
        SHARED
        IMPORTED)
set_target_properties(libavcodec
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libffmpeg/libs/${ANDROID_ABI}/libavcodec.so)

add_library(libavformat
        SHARED
        IMPORTED)
set_target_properties(libavformat
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libffmpeg/libs/${ANDROID_ABI}/libavformat.so)

add_library(libswscale
        SHARED
        IMPORTED)
set_target_properties(libswscale
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libffmpeg/libs/${ANDROID_ABI}/libswscale.so)


add_library(libswresample
        SHARED
        IMPORTED)
set_target_properties(libswresample
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libffmpeg/libs/${ANDROID_ABI}/libswresample.so)



#add_library(libavdevice
#        SHARED
#        IMPORTED)
#set_target_properties(libavdevice
#        PROPERTIES IMPORTED_LOCATION
#        ${CMAKE_SOURCE_DIR}/libffmpeg/libs/${ANDROID_ABI}/libavdevice.so)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                        ffplay
                        mediandk
                        libyuv
                        #libffmpeg
                        libavutil
                        libavcodec
                        libavformat
                        libswscale
                        libswresample
                        OpenSLES
                        android
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib})