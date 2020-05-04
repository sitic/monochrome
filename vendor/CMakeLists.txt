set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build glfw docs" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build glfw examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build glfw tests" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Generate glfw installation target" FORCE)
add_subdirectory(glfw)
set_target_properties(glfw PROPERTIES FOLDER libs)

add_library(glad glad/src/glad.c)
target_include_directories(glad PUBLIC glad/include)

add_library(imgui_glfw_gl3
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_demo.cpp
        imgui/examples/imgui_impl_glfw.cpp
        imgui/examples/imgui_impl_opengl3.cpp
        fonts/IconsFontAwesome5.cpp fonts/IconsMaterialDesignIcons.cpp
        fonts/MultiRecorderVideo.cpp fonts/DroidSans.cpp)
target_include_directories(imgui_glfw_gl3 SYSTEM PUBLIC imgui imgui/examples)
target_link_libraries(imgui_glfw_gl3 PUBLIC glad glfw OpenGL::GL)
set_target_properties(imgui_glfw_gl3 PROPERTIES FOLDER libs)
target_compile_definitions(imgui_glfw_gl3 PUBLIC "-DIMGUI_IMPL_OPENGL_LOADER_GLAD")

add_library(lodepng lodepng/lodepng.cpp lodepng/lodepng.h)
set_target_properties(lodepng PROPERTIES PUBLIC_HEADER "lodepng/lodepng.h")

add_library(ipol ipol/gaussian_conv_deriche.c ipol/gaussian_short_conv.c ipol/filter_util.c)
target_compile_definitions(ipol PUBLIC "-DNUM_SINGLE")

add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES FOLDER libs)

add_library(Eigen INTERFACE)
target_include_directories(Eigen INTERFACE Eigen)

add_library(mio INTERFACE)
target_include_directories(mio INTERFACE mio/single_include)

add_subdirectory(ghc)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
if (MSVC)
    target_compile_definitions(asio INTERFACE _WIN32_WINNT=0x0501)
endif ()

add_library(readerwriterqueue INTERFACE)
target_include_directories(readerwriterqueue INTERFACE readerwriterqueue/include)

add_library(flatbuffers INTERFACE)
target_include_directories(flatbuffers INTERFACE flatbuffers/include)

add_subdirectory(CLI11)

set_property(TARGET imgui_glfw_gl3 glad glfw fmt APPEND_STRING PROPERTY COMPILE_FLAGS " -w")