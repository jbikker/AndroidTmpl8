PROJECT(Tmpl8App C CXX)
cmake_minimum_required(VERSION 3.6.0)

add_library(native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC
        ${ANDROID_NDK}/sources/android/native_app_glue
	${CMAKE_CURRENT_SOURCE_DIR}/src/lib/soloud/include)

find_library(log-lib
        log)

add_subdirectory(src/lib/soloud)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
add_library(Tmpl8App SHARED
        src/main/cpp/template.cpp
        src/main/cpp/game.cpp
		src/main/cpp/surface.cpp
        )

target_link_libraries(Tmpl8App
        ${log-lib}
        android
	soloud
        native_app_glue
        EGL
        GLESv3
        OpenSLES
        mediandk)