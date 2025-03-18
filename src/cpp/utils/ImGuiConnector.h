#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_md.h"

namespace ImGuiConnector {
  extern ImGuiIO *io;
  extern GLFWkeyfun user_key_fun;
  extern GLFWscrollfun user_scroll_fun;

  extern ImFont *font_regular;
  extern ImFont *font_regular_large;
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

  struct markdown : imgui_md {
    ImFont *get_font() const override;
    void open_url() const override;

    void SPAN_CODE(bool e) override;
    void BLOCK_CODE(const MD_BLOCK_CODE_DETAIL *, bool e) override;
    void BLOCK_QUOTE(bool e) override;
    // ignore images
    void SPAN_IMG(const MD_SPAN_IMG_DETAIL *d, bool e) override;
  };
}  // namespace ImGuiConnector
