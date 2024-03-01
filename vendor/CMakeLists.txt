set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build glfw docs" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build glfw examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build glfw tests" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Generate glfw installation target" FORCE)
add_subdirectory(glfw)
set_target_properties(glfw PROPERTIES FOLDER libs)

add_library(glad glad/src/glad.c)
target_include_directories(glad PUBLIC glad/include)

add_library(md4c md4c/md4c.c md4c/md4c.h md4c/md4c-html.c md4c/md4c-html.h md4c/entity.c md4c/entity.h)
target_include_directories(md4c SYSTEM PUBLIC md4c)

add_library(imgui_glfw_gl3
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_demo.cpp
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
        imgui/misc/cpp/imgui_stdlib.cpp

        implot/implot.cpp implot/implot_items.cpp
        
        fonts/IconsFontAwesome5.cpp
        fonts/IconsMaterialDesignIcons.cpp
        
        imgui_md/imgui_md.cpp imgui_md/imgui_md.h
        )
target_include_directories(imgui_glfw_gl3 SYSTEM PUBLIC imgui imgui/backends imgui/misc/cpp implot imgui_md)
target_link_libraries(imgui_glfw_gl3 PUBLIC glad glfw OpenGL::GL md4c)
set_target_properties(imgui_glfw_gl3 PROPERTIES FOLDER libs)

add_library(lodepng lodepng/lodepng.cpp lodepng/lodepng.h)
set_target_properties(lodepng PROPERTIES PUBLIC_HEADER "lodepng/lodepng.h")

add_library(ipol ipol/gaussian_conv_deriche.c ipol/gaussian_short_conv.c ipol/filter_util.c)
target_compile_definitions(ipol PUBLIC "-DNUM_SINGLE")

set(FMT_INSTALL OFF CACHE BOOL "Generate fmt installation target" FORCE)
add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES FOLDER libs)

add_library(Eigen INTERFACE)
target_include_directories(Eigen INTERFACE Eigen)

add_library(mio INTERFACE)
target_include_directories(mio INTERFACE mio/single_include)

add_library(libnpy INTERFACE)
target_include_directories(libnpy INTERFACE libnpy)

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

add_library(flagset INTERFACE)
target_include_directories(flagset INTERFACE flag-set/include)

add_library(xtensor INTERFACE)
target_include_directories(xtensor INTERFACE xtensor)

add_library(cppcolormap INTERFACE)
target_include_directories(cppcolormap INTERFACE cppcolormap/include)
target_link_libraries(cppcolormap INTERFACE xtensor)

add_subdirectory(CLI11)

add_library(pugixml pugixml/src/pugixml.cpp pugixml/src/pugixml.hpp pugixml/src/pugiconfig.hpp)
target_include_directories(pugixml SYSTEM PUBLIC pugixml/src)

set_property(TARGET imgui_glfw_gl3 glad glfw fmt pugixml APPEND_STRING PROPERTY COMPILE_FLAGS " -w")