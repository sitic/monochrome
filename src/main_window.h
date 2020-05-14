#pragma once

#ifdef _WIN32  // Windows 32 and 64 bit
#include <windows.h>
#endif

#include <cstdlib>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "fonts/DroidSans.h"
#include "fonts/IconsFontAwesome5.h"
#include "fonts/IconsMaterialDesignIcons.h"

#include "globals.h"
#include "recordingwindow.h"
#include "ui.h"

namespace global {
  GLFWwindow *main_window = nullptr;

  std::vector<SharedRecordingPtr> recordings = {};
  std::queue<std::pair<SharedRecordingPtr, SharedRecordingPtr>> merge_queue;
}  // namespace global

void load_new_file(const fs::path &path) {
  fmt::print("Loading {} ...\n", path.string());

  auto rec = std::make_shared<RecordingWindow>(path);
  if (!rec->good()) {
    return;
  }
  global::recordings.push_back(rec);
  rec->open_window();
}

void load_from_queue() {
  while (auto file = global::get_file_to_load()) {
    load_new_file(file.value());
  }
  while (auto arr = global::get_rawarray3_to_load()) {
    auto r   = std::make_shared<InMemoryRecording>(arr.value());
    auto rec = std::make_shared<RecordingWindow>(r);
    if (!rec->good()) {
      global::new_ui_message("ERROR: loading file failed");
      continue;
    }

    global::recordings.push_back(rec);
    rec->open_window();

    if (auto parentName = arr.value()->meta.parentName) {
      auto parent =
          std::find_if(global::recordings.begin(), global::recordings.end(),
                       [name = parentName.value()](const auto &r) { return name == r->name(); });
      if (parent == std::end(global::recordings)) {
        global::new_ui_message(
            "Array \"{}\" has requested \"{}\" as its parent recording, but no such recording "
            "exists!",
            rec->name(), parentName.value());
      } else {
        global::merge_queue.push({rec, *parent});
      }
    }
  }
  while (!global::merge_queue.empty()) {
    auto [child, parent] = global::merge_queue.front();
    global::merge_queue.pop();
    parent->children.push_back(child);
    child->set_context(parent->window);
    child->playback = parent->playback;
    global::recordings.erase(
        std::remove_if(global::recordings.begin(), global::recordings.end(),
                       [ptr = child.get()](const auto &r) -> bool { return r.get() == ptr; }),
        global::recordings.end());
  }
}

void show_messages() {
  // Check if message window should be cleared
  global::messages.erase(std::remove_if(global::messages.begin(), global::messages.end(),
                                        [](const auto &msg) -> bool { return !msg.show; }),
                         global::messages.end());
  for (auto &msg : global::messages) {
    if (msg.show) {
      auto label = fmt::format("Message {}", msg.id);
      ImGui::SetNextWindowSizeConstraints(ImVec2(0.6f * prm::main_window_width, 0),
                                          ImVec2(FLT_MAX, FLT_MAX));
      ImGui::Begin(label.c_str(), &(msg.show), ImGuiWindowFlags_AlwaysAutoResize);
      ImGui::TextWrapped("%s", msg.msg.c_str());
      if (ImGui::Button("Ok", ImVec2(-1.0f, 0.0f))) {
        msg.show = false;
      }
      ImGui::End();
    }
  }
}

void display_loop() {
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  prm::lastframetime = glfwGetTime();
  // keep running until main window is closed
  while (!glfwWindowShouldClose(global::main_window)) {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();
    load_from_queue();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    while (glfwGetTime() < prm::lastframetime + 1.0 / prm::max_display_fps) {
    }
    prm::lastframetime += 1.0 / prm::max_display_fps;

    // ImGui::ShowDemoWindow();

    show_main_ui();

    // Check if recording window should close
    global::recordings.erase(
        std::remove_if(global::recordings.begin(), global::recordings.end(),
                       [](const auto &r) -> bool { return glfwWindowShouldClose(r->window); }),
        global::recordings.end());

    int rec_nr = 0;
    for (const auto &rec : global::recordings) {
      if (rec->active) rec->display(prm::prefilter, prm::transformation, prm::postfilter);
      for (const auto &crec : rec->children) {
        if (crec->active) crec->display(prm::prefilter, prm::transformation, prm::postfilter);
      }
      rec_nr = show_recording_ui(rec, rec_nr);
      show_export_recording_ui(rec);
    }

    show_messages();

    // Rendering
    for (const auto &recording : global::recordings) {
      recording->render();
    }
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(global::main_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(global::main_window);
    checkGlError("frame");
  }
}

void open_main_window(float font_scale = 0) {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) exit(EXIT_FAILURE);

  auto primary_monitor = glfwGetPrimaryMonitor();
  auto mode            = glfwGetVideoMode(primary_monitor);

  if (prm::main_window_width == 0) prm::main_window_width = std::max(600, mode->width / 4);
  if (prm::main_window_height == 0) prm::main_window_height = 1.5 * prm::main_window_width;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  global::main_window = glfwCreateWindow(prm::main_window_width, prm::main_window_height,
                                         "Quick Raw Video Viewer", nullptr, nullptr);
  if (!global::main_window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwSetWindowSizeCallback(global::main_window, [](GLFWwindow *window, int w, int h) {
    prm::main_window_width  = w;
    prm::main_window_height = h;
  });
  glfwSetKeyCallback(global::main_window,
                     [](GLFWwindow *window, int key, int scancode, int action, int mods) {
                       if (mods == GLFW_MOD_CONTROL && key == GLFW_KEY_Q && action == GLFW_PRESS) {
                         glfwSetWindowShouldClose(window, GLFW_TRUE);
                       }
                     });
  glfwSetDropCallback(global::main_window, [](GLFWwindow *window, int count, const char **paths) {
    for (int i = 0; i < count; i++) {
      load_new_file(paths[i]);
    }
  });
  glfwMakeContextCurrent(global::main_window);
  // wait until the current frame has been drawn before drawing the next one
  glfwSwapInterval(0);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize OpenGL loader!\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // Disable .ini generation/loading for now
  io.IniFilename = nullptr;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // TODO: Better HIDIP handling
  if (font_scale == 0) {
    float xscale, yscale;
    glfwGetMonitorContentScale(primary_monitor, &xscale, &yscale);
    font_scale = std::max(xscale, yscale);
  }
  ImGui::GetStyle().ScaleAllSizes(font_scale);
  ImGui::GetStyle().FrameRounding = 3;

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(global::main_window, true);
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
  ImFontAtlas::GlyphRangesBuilder builder;
  builder.AddText(u8"σπ");
  builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
  static ImVector<ImWchar> ranges;
  builder.BuildRanges(&ranges);
  ImFontConfig font_config;
  font_config.OversampleH = 3;
  font_config.OversampleV = 2;
  //font_config.PixelSnapH = true;
  io.Fonts->AddFontFromMemoryCompressedTTF(fonts::DroidSans_compressed_data,
                                           fonts::DroidSans_compressed_size, 14, &font_config,
                                           ranges.Data);
  ImFontConfig icons_config;
  icons_config.MergeMode  = true;
  icons_config.PixelSnapH = true;

  static const ImWchar fontawesome_icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(fonts::fontawesome5_solid_compressed_data,
                                           fonts::fontawesome5_solid_compressed_size, 11,
                                           &icons_config, fontawesome_icons_ranges);
  static const ImWchar materialdesignicons_icons_ranges[] = {ICON_MIN_MDI, ICON_MAX_MDI, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(fonts::materialdesignicons_compressed_data,
                                           fonts::materialdesignicons_compressed_size, 11,
                                           &icons_config, materialdesignicons_icons_ranges);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  // ImFont* font =
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
  // NULL, io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);

  add_window_icon(global::main_window);

  for (auto cmap : prm::cmaps) {
    GLuint tex = GL_FALSE;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    auto cdata = get_colormapdata(cmap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, cdata.size() / 3, 1, 0, GL_RGB, GL_FLOAT,
                 cdata.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // GL_LINEAR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // GL_LINEAR

    prm::cmap_texs[cmap] = tex;
  }
}