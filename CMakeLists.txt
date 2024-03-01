cmake_minimum_required(VERSION 3.12)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cmake)
include(GitVersionDetect)

project(Monochrome
        HOMEPAGE_URL "https://github.com/sitic/monochrome"
        DESCRIPTION "Monochrome is a video viewer for raw monochromatic video data."
        VERSION ${GITVERSIONDETECT_VERSION_MAJOR}.${GITVERSIONDETECT_VERSION_MINOR}.${GITVERSIONDETECT_VERSION_PATCH}
        LANGUAGES C CXX)

option(MC_FORCE_TCP_IPC "Force use of TCP for IPC" OFF)
option(STATIC_GCC "Build with static libstdc++ when using gcc" OFF)

# default to Release build
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug, Release or RelWithDebInfo" FORCE)
endif ()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

include(CMakeRC)
include(GNUInstallDirs)

# Build for oldest supported glibc version if STATIC_GCC is set
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

# add third party libraries
add_subdirectory(vendor)
include_directories(SYSTEM vendor)

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
    # OpenGL was deprecated on MacOS, ignore warnings
    add_definitions(-DGL_SILENCE_DEPRECATION)
    # For backwards compatibility with MacOS High Sierra, this is not a good solution and should be avoided ...
    # https://stackoverflow.com/a/53868971/1393377
    # TODO: remove when not longer supporting MacOS 10.13 / 10.14
    add_definitions(-D_LIBCPP_DISABLE_AVAILABILITY)
endif ()

file(GLOB SHADERS "src/shaders/*.glsl")
cmrc_add_resource_library(monochrome-resources
                          ALIAS monochrome::rc
                          NAMESPACE rc
                          ${SHADERS}
                          "assets/Monochrome_256x256.png"
                          "README.md"
                          "vendor/fonts/NotoSansDisplay-Regular.ttf"
                          "vendor/fonts/NotoSansDisplay-Bold.ttf"
                          "vendor/fonts/FiraCode-Regular.ttf"
)

set(SOURCES
        src/cpp/main.cpp

        src/cpp/globals.cpp
        src/cpp/globals.h
        src/cpp/ipc.cpp
        src/cpp/ipc.h
        src/cpp/main_window.h
        src/cpp/recording.cpp
        src/cpp/recording.h
        src/cpp/ui.h
        src/cpp/ui.cpp
        src/cpp/recordingwindow.cpp
        src/cpp/recordingwindow.h
        src/cpp/recordingwindow_helpers.cpp
        src/cpp/recordingwindow_helpers.h
        src/cpp/transformations.cpp
        src/cpp/transformations.h
        src/cpp/keybindings.cpp
        src/cpp/keybindings.h
        src/cpp/prm.h
        src/cpp/prm.cpp

        src/cpp/ui/main.h
        src/cpp/ui/export.h
        src/cpp/ui/recording.h
        src/cpp/ui/recording_controls.h
        src/cpp/ui/recording_histogram.h
        src/cpp/ui/recording_points.h
        src/cpp/ui/recording_traces.h

        src/cpp/fileformats/AbstractFile.h
        src/cpp/fileformats/all_formats.cpp
        src/cpp/fileformats/all_formats.h
        src/cpp/fileformats/BmpFile.h
        src/cpp/fileformats/BmpFileParser.h
        src/cpp/fileformats/InMemoryFile.h
        src/cpp/fileformats/RawFile.h

        src/cpp/utils/colormap.cpp
        src/cpp/utils/colormap.h
        src/cpp/utils/definitions.h
        src/cpp/utils/ImGuiConnector.h
        src/cpp/utils/ImGuiConnector.cpp
        src/cpp/utils/iterators.h
        src/cpp/utils/plot_utils.h
        src/cpp/utils/utils.cpp
        src/cpp/utils/utils.h
        src/cpp/utils/vectors.h
        src/cpp/utils/videorecorder.h
        src/cpp/utils/settings.h
        src/cpp/utils/settings.cpp
)

if(APPLE)
    set(SOURCES ${SOURCES}
        src/cpp/macOS/GLFWCustomDelegate.h
        src/cpp/macOS/GLFWCustomDelegate.mm
    )
    set(MONOCHROME_ICONS
        ${PROJECT_SOURCE_DIR}/assets/Monochrome.icns
        ${PROJECT_SOURCE_DIR}/assets/npy-logo.icns
    )
    set_source_files_properties(${MONOCHROME_ICONS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(Monochrome ${MONOCHROME_ICONS} ${SOURCES})
    set_target_properties(Monochrome PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/assets/Info.plist
    )
elseif(MSVC AND NOT SKBUILD)
    add_executable(Monochrome WIN32 ${SOURCES} ${PROJECT_SOURCE_DIR}/assets/Monochrome.rc)
    target_compile_definitions(Monochrome PRIVATE USE_WIN32_MAIN)
else()
    add_executable(Monochrome ${SOURCES})
endif()

target_link_libraries(Monochrome PRIVATE
        CLI11::CLI11 imgui_glfw_gl3 fmt lodepng Eigen ipol asio readerwriterqueue flatbuffers
        ghc_filesystem mio libnpy pugixml flagset cppcolormap monochrome::rc)
target_include_directories(Monochrome PRIVATE src/cpp src/schema)
target_compile_definitions(Monochrome PRIVATE "-DGLFW_INCLUDE_NONE")

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
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION ./)

# Do platform specific post target stuff
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Install desktop entry and icon
    install(FILES assets/Monochrome.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES assets/Monochrome_256x256.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME Monochrome.png)
endif()

if (NOT SKBUILD)
    if (APPLE)
        set(APP_NAME Monochrome)
        set(CPACK_BUNDLE_NAME Monochrome)
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_BUNDLE_ICON ${PROJECT_SOURCE_DIR}/assets/Monochrome.icns)
        set(CPACK_PACKAGE_FILE_NAME "${APP_NAME}-${CMAKE_PROJECT_VERSION}_macOS")
    elseif(WIN32)
        set(CPACK_GENERATOR "ZIP")
        set(CPACK_PACKAGE_FILE_NAME "Monochrome-${CMAKE_PROJECT_VERSION}_Windows")
    else()
        set(CPACK_GENERATOR "TGZ")
        set(CPACK_PACKAGE_FILE_NAME "Monochrome-${CMAKE_PROJECT_VERSION}_Linux")
    endif()
    include(CPack)
endif()
