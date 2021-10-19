cmake_minimum_required(VERSION 3.1)

project(Monochrome C CXX)

option(MC_FORCE_TCP_IPC "Force use of TCP for IPC" OFF)
option(STATIC_GCC "Build with static libstdc++ when using gcc" OFF)

# default to Release build
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug, Release or RelWithDebInfo" FORCE)
endif ()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/vendor/cmake)
include(CMakeRC)

include(GNUInstallDirs)

if (CMAKE_COMPILER_IS_GNUCXX AND STATIC_GCC)
    # Check if it works with  `objdump -p <Executable>`
    set(GLIBC_VER "2.23")
    message(STATUS "Force linking glibc version ${GLIBC_VER}")
    set(GLIBC_HEADERFILE "${CMAKE_SOURCE_DIR}/vendor/glibc_version_header/version_headers/x64/force_link_glibc_${GLIBC_VER}.h")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -include ${GLIBC_HEADERFILE}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include ${GLIBC_HEADERFILE}")
endif ()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL)

# add third party libraries
add_subdirectory(vendor)
include_directories(SYSTEM vendor)

# use ccache if available for faster compile times
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    message(STATUS "Enabled ccache: ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif ()


# require c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    # Fix strange windows defaults
    set(CMAKE_WIN32_EXECUTABLE 0 CACHE INTERNAL "")
    set(COMPILER_WARNINGS "/wd4018 /wd4244 /wd4305 /wd4267")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_WARNINGS} /utf-8")

    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

if (APPLE)
    # OpenGL was depricated on MacOS, ignore warnings
    add_definitions(-DGL_SILENCE_DEPRECATION)
    # For backwards compatibility with MacOS High Sierra, this is not a good solution and should be avoided ...
    # https://stackoverflow.com/a/53868971/1393377
    # TODO: remove when not longer supporting MacOS 10.13 / 10.14
    add_definitions(-D_LIBCPP_DISABLE_AVAILABILITY)
endif ()

file(GLOB SHADERS "src/shaders/*.glsl")
cmrc_add_resource_library(monochrome-resources ALIAS qvv::rc NAMESPACE rc ${SHADERS} "vendor/MultiRecorderVideo.png")

add_executable(Monochrome src/main.cpp src/utils/utils.h src/fileformats/BmpFileParser.h src/recording.h src/utils/vectors.h
        src/utils/videorecorder.h src/recordingwindow.h src/transformations.h src/main_window.h src/globals.cpp src/globals.h
        src/ipc.h src/globals.cpp src/globals.h src/ipc.cpp src/recordingwindow.cpp src/utils/utils.cpp src/transformations.cpp
        src/recording.cpp src/ui.h src/utils/colormap.cpp src/utils/colormap.h src/utils/iterators.h src/recordingwindow_helpers.cpp
        src/recordingwindow_helpers.h src/fileformats/AbstractFile.h src/fileformats/all_formats.h
        src/fileformats/all_formats.cpp src/utils/definitions.h src/utils/plot_utils.h)
target_link_libraries(Monochrome PRIVATE
        CLI11::CLI11 imgui_glfw_gl3 fmt lodepng Eigen ipol asio readerwriterqueue flatbuffers ghc_filesystem mio libnpy
        pugixml flagset cppcolormap qvv::rc)
target_include_directories(Monochrome PRIVATE src)
if (MC_FORCE_TCP_IPC)
    target_compile_definitions(Monochrome PRIVATE "-DMC_FORCE_TCP_IPC")
endif ()
if (CMAKE_COMPILER_IS_GNUCXX)
    if (STATIC_GCC)
        target_link_libraries(Monochrome PRIVATE -static-libgcc -static-libstdc++)
    else ()
        # Hotfix so that is shows up in the file explorer as executable, see https://stackoverflow.com/questions/41398444/
        target_link_options(Monochrome PRIVATE "-no-pie")
    endif ()
endif ()

install(TARGETS Monochrome
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Do platform specific post target stuff
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Install desktop entry and icon
    install(FILES vendor/Monochrome.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES vendor/MultiRecorderVideo.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
