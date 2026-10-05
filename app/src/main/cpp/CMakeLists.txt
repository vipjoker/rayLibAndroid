cmake_minimum_required(VERSION 3.22.1)
project("${APP_LIB_NAME}")

set(CMAKE_C_STANDARD 99)

add_library(raylib STATIC IMPORTED)



set_target_properties(raylib PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/raylib/include"
        INTERFACE_LINK_LIBRARIES "android;log;EGL;GLESv2;OpenSLES;m;atomic;dl"
        INTERFACE_LINK_OPTIONS "-Wl,--wrap=fopen"
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libraylib.a"
)

# Subdirectories
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/raymob)

# App source files
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/*.c" "${CMAKE_SOURCE_DIR}/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/deps/.*")
list(APPEND SOURCES "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")

add_library(${APP_LIB_NAME} SHARED ${SOURCES})

target_compile_definitions(${APP_LIB_NAME} PRIVATE PLATFORM_ANDROID)

target_include_directories(${APP_LIB_NAME} PRIVATE
        "${ANDROID_NDK}/sources/android/native_app_glue"
        "${CMAKE_SOURCE_DIR}/deps/raymob"
)

target_link_libraries(${APP_LIB_NAME} raylib raymoblib)
