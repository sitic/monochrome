#include <GLFW/glfw3.h>
#include "imgui.h"

namespace ImGuiConnector {
  extern ImGuiIO *io;
  extern GLFWkeyfun user_key_fun;
  extern GLFWscrollfun user_scroll_fun;

  extern ImFont *font_regular;
  extern ImFont *font_bold;
  extern ImFont *font_bold_large;
  extern ImFont *font_code;

  void Init(GLFWwindow *window,
            GLFWmonitor *primary_monitor,
            float font_scale,
            GLFWkeyfun key_fun       = nullptr,
            GLFWscrollfun scroll_fun = nullptr);

  void NewFrame();

  void Render(GLFWwindow *window, const ImVec4 &clear_color);

  void Shutdown();
}  // namespace ImGuiConnector