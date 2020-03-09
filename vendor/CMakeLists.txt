set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "Build glfw docs" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build glfw examples" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "Build glfw tests" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "Generate glfw installation target" FORCE)
add_subdirectory(glfw)
set_target_properties(glfw PROPERTIES FOLDER libs)

add_library(imgui_glfw_gl2
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_demo.cpp
        imgui/examples/imgui_impl_glfw.cpp
        imgui/examples/imgui_impl_opengl2.cpp
        fonts/IconsFontAwesome5.cpp fonts/IconsMaterialDesignIcons.cpp
        fonts/MultiRecorderVideo.cpp)
target_include_directories(imgui_glfw_gl2 SYSTEM PUBLIC imgui imgui/examples)
target_link_libraries(imgui_glfw_gl2 PUBLIC glfw OpenGL::GL)
set_target_properties(imgui_glfw_gl2 PROPERTIES FOLDER libs)

add_library(lodepng lodepng/lodepng.cpp lodepng/lodepng.h)
set_target_properties(lodepng PROPERTIES PUBLIC_HEADER "lodepng/lodepng.h")

add_library(ipol ipol/gaussian_conv_deriche.c ipol/gaussian_short_conv.c ipol/filter_util.c)
target_compile_definitions(ipol PUBLIC "-DNUM_SINGLE")

# fmt
add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES FOLDER libs)

add_library(Eigen INTERFACE)
target_include_directories(Eigen INTERFACE Eigen)

add_subdirectory(CLI11)

set_property(TARGET imgui_glfw_gl2 glfw fmt APPEND_STRING PROPERTY COMPILE_FLAGS " -w")