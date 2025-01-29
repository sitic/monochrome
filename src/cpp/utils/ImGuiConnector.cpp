#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmrc/cmrc.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "implot.h"

#include "ImGuiConnector.h"

#include "fonts/IconsFontAwesome5.h"
#include "fonts/IconsMaterialDesignIcons.h"

CMRC_DECLARE(rc);

namespace {
  ImFont* get_imgui_font(std::string name,
                         float pixel_size,
                         ImFontConfig* font_cfg      = nullptr,
                         const ImWchar* glyph_ranges = nullptr) {
    auto fs       = cmrc::rc::get_filesystem();
    auto fontdata = fs.open("vendor/fonts/" + name);
    ImGuiIO& io   = ImGui::GetIO();

    auto font = io.Fonts->AddFontFromMemoryTTF((void*)(fontdata.begin()), fontdata.size(),
                                               pixel_size, font_cfg, glyph_ranges);

    return font;
  }
}  // namespace
namespace ImGuiConnector {
  ImGuiIO* io                   = nullptr;
  GLFWkeyfun user_key_fun       = nullptr;
  GLFWscrollfun user_scroll_fun = nullptr;

  ImFont* font_regular    = nullptr;
  ImFont* font_bold       = nullptr;
  ImFont* font_bold_large = nullptr;
  ImFont* font_code       = nullptr;

  void Init(GLFWwindow* window,
            GLFWmonitor* primary_monitor,
            float font_scale,
            GLFWkeyfun key_fun,
            GLFWscrollfun scroll_fun) {
    if (key_fun) user_key_fun = key_fun;
    if (scroll_fun) user_scroll_fun = scroll_fun;

    ImGui::CreateContext();
    ImPlot::CreateContext();
    io = &(ImGui::GetIO());
    // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Disable .ini generation/loading for now
    io->IniFilename = nullptr;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

#ifdef __APPLE__
    // to prevent 1200x800 from becoming 2400x1600
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
    if (font_scale == 0) {
#ifdef __APPLE__
      font_scale = 1;
#else
      float xscale, yscale;
      glfwGetMonitorContentScale(primary_monitor, &xscale, &yscale);
      font_scale = std::max(xscale, yscale);
#endif
    }
    ImGui::GetStyle().ScaleAllSizes(font_scale);
    ImGui::GetStyle().FrameRounding = 3;

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    glfwSetWindowFocusCallback(window, ImGui_ImplGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(window, ImGui_ImplGlfw_CursorEnterCallback);
    glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
    glfwSetCursorPosCallback(window, ImGui_ImplGlfw_CursorPosCallback);
    // Scroll Callback
    if (user_scroll_fun) {
      GLFWscrollfun callback = [](GLFWwindow* window, double xoffset, double yoffset) {
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

        // if imgui wants the input, don't dispatch it to our app
        if (io && !io->WantCaptureKeyboard && user_scroll_fun) {
          user_scroll_fun(window, xoffset, yoffset);
        }
      };
      glfwSetScrollCallback(window, callback);
    } else {
      glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
    }
    // KeyCallback
    if (user_key_fun) {
      GLFWkeyfun callback = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

        // if imgui wants the input, don't dispatch it to our app
        if (io && !io->WantCaptureKeyboard && user_key_fun) {
          user_key_fun(window, key, scancode, action, mods);
        }
      };
      glfwSetKeyCallback(window, callback);
    } else {
      glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
    }
    glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
    glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);
    ImGui_ImplOpenGL3_Init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can
    // also load multiple fonts and use ImGui::PushFont()/PopFont() to select
    // them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you
    // need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please
    // handle those errors in your application (e.g. use an assertion, or
    // display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and
    // stored into a texture when calling
    // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame
    // below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string
    // literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    ImFontGlyphRangesBuilder builder;
    builder.AddText(u8"σπ—");
    builder.AddRanges(io->Fonts->GetGlyphRangesDefault());
    static ImVector<ImWchar> ranges;
    builder.BuildRanges(&ranges);

    ImFontConfig font_config;
    font_config.OversampleH = 3;
    font_config.OversampleV = 2;
    //font_config.PixelSnapH = true;
    font_config.FontDataOwnedByAtlas = false;

    // float font_size_regular = std::ceil(14.f * font_scale);
    // float font_size_large   = std::ceil(18.f * font_scale);
    // float font_size_icon    = std::ceil(11.f * font_scale);
    float font_size_regular = 16.f * font_scale;
    float font_size_large   = 18.f * font_scale;
    float font_size_icon    = 12.5f * font_scale;
    font_regular =
        get_imgui_font("NotoSansDisplay-Regular.ttf", font_size_regular, &font_config, ranges.Data);

    ImFontConfig icons_config;
    icons_config.MergeMode                          = true;
    icons_config.PixelSnapH                         = true;
    static const ImWchar fontawesome_icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    io->Fonts->AddFontFromMemoryCompressedTTF(
        fonts::fontawesome5_solid_compressed_data, fonts::fontawesome5_solid_compressed_size,
        font_size_icon, &icons_config, fontawesome_icons_ranges);
    static const ImWchar materialdesignicons_icons_ranges[] = {ICON_MIN_MDI, ICON_MAX_MDI, 0};
    io->Fonts->AddFontFromMemoryCompressedTTF(
        fonts::materialdesignicons_compressed_data, fonts::materialdesignicons_compressed_size,
        font_size_icon, &icons_config, materialdesignicons_icons_ranges);

    font_bold =
        get_imgui_font("NotoSansDisplay-Bold.ttf", font_size_regular, &font_config, ranges.Data);
    font_bold_large =
        get_imgui_font("NotoSansDisplay-Bold.ttf", font_size_large, &font_config, ranges.Data);
    font_code = get_imgui_font("FiraCode-Regular.ttf", font_size_regular, &font_config, ranges.Data);
  }

  void NewFrame() {
    if (!io) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
  }

  void Render(GLFWwindow* window, const ImVec4& clear_color) {
    if (!io) return;

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  void Shutdown() {
    if (!io) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
  }
}  // namespace ImGuiConnector