cmake_minimum_required(VERSION 3.1)

project(quickViewer CXX)

# default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug, Release or RelWithDebInfo" FORCE)
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL)

# add third party libraries
add_subdirectory(vendor)
include_directories(SYSTEM vendor)

# use ccache if available for faster compile times
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Enabled ccache: ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()


# require c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Fix strange windows defaults
    set(CMAKE_WIN32_EXECUTABLE 0 CACHE INTERNAL "")
    set(COMPILER_WARNINGS "/wd4018 /wd4244 /wd4305 /wd4267")
    set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} ${COMPILER_WARNINGS} /utf-8")

    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(APPLE)
    # OpenGL was depricated on MacOS, ignore warnings
    add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

add_executable(quickViewer src/main.cpp src/definitions.h src/utils.h src/bmp.h src/recording.h src/vectors.h src/filereader.h src/videorecorder.h)
target_link_libraries(quickViewer imgui_glfw_gl2 fmt lodepng)
if(CMAKE_COMPILER_IS_GNUCXX)
    # Hotfix so that is shows up in the file explorer as executable, see https://stackoverflow.com/questions/41398444/
    target_link_options(quickViewer PRIVATE "-no-pie")
endif()
