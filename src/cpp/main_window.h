#pragma once

#ifdef _WIN32  // Windows 32 and 64 bit
#include <windows.h>
#endif

#include <cstdlib>
#include <vector>
#include <thread>

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
  // Queue Element: [child, parent, as_flow]
  std::queue<std::tuple<SharedRecordingPtr, SharedRecordingPtr, bool>> merge_queue;
}  // namespace global

namespace ImGuiConnector {
  ImGuiIO *io                   = nullptr;
  GLFWkeyfun user_key_fun       = nullptr;
  GLFWscrollfun user_scroll_fun = nullptr;

  void Init(GLFWwindow *window,
            GLFWmonitor *primary_monitor,
            float font_scale,
            GLFWkeyfun key_fun       = nullptr,
            GLFWscrollfun scroll_fun = nullptr) {
    if (key_fun) user_key_fun = key_fun;
    if (scroll_fun) user_scroll_fun = scroll_fun;

    ImGui::CreateContext();
    ImPlot::CreateContext();
    io = &(ImGui::GetIO());
    (void)io;
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
    glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
    if (user_scroll_fun) {
      GLFWscrollfun callback = [](GLFWwindow *window, double xoffset, double yoffset) {
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
    if (user_key_fun) {
      GLFWkeyfun callback = [](GLFWwindow *window, int key, int scancode, int action, int mods) {
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
    builder.AddText(u8"σπ");
    builder.AddRanges(io->Fonts->GetGlyphRangesDefault());
    static ImVector<ImWchar> ranges;
    builder.BuildRanges(&ranges);
    ImFontConfig font_config;
    font_config.OversampleH = 3;
    font_config.OversampleV = 2;
    //font_config.PixelSnapH = true;
    io->Fonts->AddFontFromMemoryCompressedTTF(
        fonts::DroidSans_compressed_data, fonts::DroidSans_compressed_size,
        std::ceil(14.f * font_scale), &font_config, ranges.Data);
    ImFontConfig icons_config;
    icons_config.MergeMode  = true;
    icons_config.PixelSnapH = true;

    static const ImWchar fontawesome_icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    io->Fonts->AddFontFromMemoryCompressedTTF(
        fonts::fontawesome5_solid_compressed_data, fonts::fontawesome5_solid_compressed_size,
        std::ceil(11.f * font_scale), &icons_config, fontawesome_icons_ranges);
    static const ImWchar materialdesignicons_icons_ranges[] = {ICON_MIN_MDI, ICON_MAX_MDI, 0};
    io->Fonts->AddFontFromMemoryCompressedTTF(
        fonts::materialdesignicons_compressed_data, fonts::materialdesignicons_compressed_size,
        std::ceil(11.f * font_scale), &icons_config, materialdesignicons_icons_ranges);
  }

  void NewFrame() {
    if (!io) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
  }

  void Render(GLFWwindow *window, const ImVec4 &clear_color) {
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

void load_new_file(std::shared_ptr<AbstractFile> file,
                   std::optional<std::string> parentName = std::nullopt) {
  if (!file || !file->good()) return;
  auto findParent = [](const std::string &parent_name) {
    auto it = std::find_if(global::recordings.begin(), global::recordings.end(),
                           [name = parent_name](const auto &r) { return name == r->name(); });
    if (it == std::end(global::recordings))
      return SharedRecordingPtr(nullptr);
    else
      return *it;
  };

  if (!file->is_flow()) {
    auto rec = std::make_shared<RecordingWindow>(file);
    global::recordings.push_back(rec);
    rec->open_window();

    if (parentName) {
      auto parent = findParent(parentName.value());
      if (!parent) {
        global::new_ui_message(
            "Array \"{}\" has requested \"{}\" as its parent recording, but no such recording "
            "exists!",
            rec->name(), parentName.value());
      } else {
        global::merge_queue.push({rec, parent, false});
      }
    } else if (global::recordings.size() / 3 + 1 != prm::main_window_multipier) {
      prm::main_window_multipier = global::recordings.size() / 3 + 1;
      prm::main_window_multipier = std::clamp(prm::main_window_multipier, 1, 3);
      glfwSetWindowSize(global::main_window, prm::main_window_multipier * prm::main_window_width,
                        prm::main_window_height);
    }
  } else {
    SharedRecordingPtr parent;
    if (parentName) {
      parent = findParent(parentName.value());
    }
    if (!parent) {
      if (parentName) {
        global::new_ui_message(
            "Failed to add flow to recording, no recording with name \"{}\" exists!",
            parentName.value());
        return;
      } else if (global::recordings.empty()) {
        global::new_ui_message(
            "You loaded flow vectors before any regular recording, load regular recording first");
        return;
      } else {
        parent = global::recordings.back();
      }
    }

    auto rec = std::make_shared<Recording>(file);
    parent->add_flow(rec);
  }
}

void load_new_file(const fs::path &path) {
  fmt::print("Loading {} ...\n", path.string());
  load_new_file(file_factory(path));
}

void load_from_queue() {
  /* Check all our queues for new elements */

  /* File paths */
  if (auto filepath = global::get_file_to_load()) {
    load_new_file(filepath.value());
  }

  /* InMemory Arrays */
  if (auto arr = global::get_rawarray3_to_load()) {
    auto file = std::make_shared<InMemoryFile>(arr.value());
    load_new_file(file, arr.value()->meta.parentName);
  }

  /* Point videos */
  if (auto pointsvideo = global::get_pointsvideo_to_load()) {
    if (global::recordings.empty()) {
      global::new_ui_message("A video needs to be opened before points lists can be displayed");
    } else {
      auto rec         = global::recordings.back();
      auto parent_name = pointsvideo.value()->parent_name;
      rec->add_points_video(pointsvideo.value());
    }
  }

  /* Global merge queue */
  if (!global::merge_queue.empty()) {
    auto [child, parent, as_flow] = global::merge_queue.front();
    global::merge_queue.pop();
    if (!as_flow) {
      parent->children.push_back(child);
      child->set_context(parent->window);
      child->playback = parent->playback;
    } else {
      if (child->file()->set_flow(true)) {
        child->set_context(parent->window);
        parent->add_flow(child);
      }
    }
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
    ImGuiConnector::NewFrame();

    // Sleep until we need to wake up for desired framerate
    double time_per_frame = 1.0 / prm::display_fps;
    prm::lastframetime += time_per_frame;
    std::chrono::duration<double> sleep_duration(prm::lastframetime - glfwGetTime());
    if (sleep_duration.count() < -100 * time_per_frame &&
        sleep_duration.count() > -10000 * time_per_frame) {
      // Avoid a big jump if the current framerate is much slower than the desired framerate,
      // but not if the framerate is free running (during .mp4 export)
      prm::lastframetime = glfwGetTime();
    }
    std::this_thread::sleep_for(sleep_duration);

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
      rec_nr = show_recording_ui(rec, rec_nr);
      show_export_recording_ui(rec);
    }

    show_messages();

    // Rendering
    for (const auto &recording : global::recordings) {
      recording->render();
    }
    ImGuiConnector::Render(global::main_window, clear_color);
    glfwSwapBuffers(global::main_window);
    checkGlError("frame");
  }
}

void main_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (mods == GLFW_MOD_CONTROL && key == GLFW_KEY_Q && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  } else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE) {
    int steps = (mods == GLFW_MOD_SHIFT) ? 10 : 1;
    for (const auto &rec : global::recordings) {
      int t = rec->current_frame();
      rec->playback.set_next(t + steps);
    }
  } else if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE) {
    int steps = (mods == GLFW_MOD_SHIFT) ? 10 : 1;
    for (const auto &rec : global::recordings) {
      int t = rec->current_frame();
      rec->playback.set_next(t - steps);
    }
  } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    prm::playbackCtrl.toggle_play_pause();
  } else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    for (const auto &rec : global::recordings) {
      rec->playback.set_next(0);
    }
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
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
  global::main_window = glfwCreateWindow(prm::main_window_multipier * prm::main_window_width,
                                         prm::main_window_height, "Monochrome", nullptr, nullptr);
  if (!global::main_window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwGetFramebufferSize(global::main_window, &prm::main_window_width, &prm::main_window_height);
  glfwSetWindowSizeCallback(global::main_window, [](GLFWwindow *window, int w, int h) {
    prm::main_window_width  = w / prm::main_window_multipier;
    prm::main_window_height = h;
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

  ImGuiConnector::Init(global::main_window, primary_monitor, font_scale, main_key_callback);

  add_window_icon(global::main_window);

  // Initialize colormap textures
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