#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_md.h"

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

void system_open_url(const std::string &url);

struct markdown_cls : public imgui_md {
  ImFont *get_font() const override {
    if (m_is_table_header) {
      return ImGuiConnector::font_bold_large;
    }

    if (m_is_code) {
      return ImGuiConnector::font_code;
    }

    switch (m_hlevel) {
      case 0:
        return m_is_strong ? ImGuiConnector::font_bold : ImGuiConnector::font_regular;
      case 1:
        return ImGuiConnector::font_bold_large;
      case 2:
        return ImGuiConnector::font_bold_large;
      default:
        return ImGuiConnector::font_bold;
    }
  };

  void open_url() const override { system_open_url(m_href); }

  void SPAN_CODE(bool e) override {
    if (e) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8, 0.8, 0.8, 1));
    } else {
      ImGui::PopStyleColor();
    }
  }

  void BLOCK_CODE(const MD_BLOCK_CODE_DETAIL *, bool e) override {
    m_is_code = e;
    if (e) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8, 0.8, 0.8, 1));
      ImGui::Indent();
    } else {
      ImGui::Unindent();
      ImGui::PopStyleColor();
    }
  }

  void BLOCK_QUOTE(bool e) override {
    if (e) {
      ImGui::Indent();
    } else {
      ImGui::Unindent();
    }
  }

  // ignore images
  void SPAN_IMG(const MD_SPAN_IMG_DETAIL *d, bool e) override { m_is_image = e; }
};