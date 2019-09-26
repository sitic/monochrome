set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "Build glfw docs" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build glfw examples" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "Build glfw tests" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "Generate glfw installation target" FORCE)
add_subdirectory(glfw)
set_target_properties(glfw PROPERTIES FOLDER libs)

# All viewer library cpp files which don't have their own target
add_library(viewerlibs OBJECT
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_demo.cpp
        imgui/examples/imgui_impl_glfw.cpp
        imgui/examples/imgui_impl_opengl2.cpp)
target_include_directories(viewerlibs SYSTEM PUBLIC glfw/include)
target_include_directories(viewerlibs SYSTEM PRIVATE imgui)
set_target_properties(viewerlibs PROPERTIES FOLDER libs)
set_property(TARGET viewerlibs glfw APPEND_STRING PROPERTY COMPILE_FLAGS " -w")

set(VIEWERLIBS $<TARGET_OBJECTS:viewerlibs> PARENT_SCOPE)