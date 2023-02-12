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

#include "utils/ImGuiConnector.h"

#include "globals.h"
#include "recordingwindow.h"
#include "ui.h"
#include "keybindings.h"

namespace global {
  GLFWwindow *main_window = nullptr;

  std::vector<SharedRecordingPtr> recordings = {};
  // Queue Element: [child, parent, as_flow]
  std::queue<std::tuple<SharedRecordingPtr, SharedRecordingPtr, bool>> merge_queue;
}  // namespace global

SharedRecordingPtr find_parent_recording(const std::string &parent_name) {
  // If the parent_name is empty we default to using the last loaded video
  if (parent_name.empty() && !global::recordings.empty()) return global::recordings.back();

  // Find the last video with the name `parent_name`
  auto it = std::find_if(global::recordings.rbegin(), global::recordings.rend(),
                         [name = parent_name](const auto &r) { return name == r->name(); });
  if (it == std::rend(global::recordings))
    return SharedRecordingPtr(nullptr);
  else
    return *it;
}

void load_new_file(std::shared_ptr<AbstractFile> file,
                   std::optional<std::string> parentName = std::nullopt) {
  if (!file || !file->good()) return;

  if (file->is_flow()) {  // Flow array
    SharedRecordingPtr parent;
    if (!parentName) {
      parentName = "";
    }
    parent = find_parent_recording(parentName.value());
    if (!parent) {
      std::string err_msg =
          global::recordings.empty()
              ? "ERROR: Failed to add flow to recording, no recording with name \"{}\" exists!"
              : "ERROR: Failed to add flow to recording, you need to load a recording first!";
      global::new_ui_message(err_msg, parentName.value());
      return;
    }

    auto rec = std::make_shared<Recording>(file);
    parent->add_flow(rec);
  } else {
    auto rec = std::make_shared<RecordingWindow>(file);
    if (!global::recordings.empty()) {
      rec->playback.synchronize_with(global::recordings.back()->playback, false);
    }
    global::recordings.push_back(rec);
    rec->open_window();

    if (parentName) {
      auto parent = find_parent_recording(parentName.value());
      if (!parent) {
        if (global::recordings.empty())
          global::new_ui_message("ERROR: You need to load a video first before adding a layer");
        else
          global::new_ui_message(
              "ERROR: Video \"{}\" has requested \"{}\" as its parent recording, but no such "
              "recording exists!",
              rec->name(), parentName.value());
      } else {
        global::merge_queue.push({rec, parent, false});
      }
    } else if (auto num_recs = global::recordings.size();
               num_recs / 3 + 1 != prm::main_window_multipier) {
      // TODO: this should be removed in the UI rewrite
      prm::main_window_multipier = num_recs / 3 + 1;
      prm::main_window_multipier = std::clamp(prm::main_window_multipier, 1, 3);
      glfwSetWindowSize(global::main_window,
                        prm::main_window_multipier * ImGui::GetMainViewport()->Size[0],
                        ImGui::GetMainViewport()->Size[1]);
    }
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
    return;  // there might be some order of operations issues for IPC if we don't return here
  }

  /* InMemory Arrays */
  if (auto arr = global::get_rawarray3_to_load()) {
    auto file = std::make_shared<InMemoryFile>(arr.value());
    load_new_file(file, arr.value()->meta.parentName);
    return;  // there might be some order of operations issues for IPC if we don't return here
  }

  /* Point Videos */
  if (auto pointsvideo = global::get_pointsvideo_to_load()) {
    if (global::recordings.empty()) {
      global::new_ui_message(
          "ERROR: A video needs to be opened before points lists can be displayed");
    } else {
      auto parent_name = pointsvideo.value()->parent_name;
      auto parent      = find_parent_recording(parent_name);
      if (!parent) {
        global::new_ui_message(
            "ERROR: Points Array \"{}\" has requested \"{}\" as its parent recording, but no such "
            "recording exists!",
            pointsvideo.value()->name, parent_name);
      } else {
        parent->add_points_video(pointsvideo.value());
      }
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
  // Show messages
  for (auto &msg : global::messages) {
    if (msg.show) {
      auto label = fmt::format("Message {}", msg.id);
      ImGui::SetNextWindowSizeConstraints(ImVec2(0.4f * ImGui::GetMainViewport()->Size[0], 0),
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

  auto key_callback = [](GLFWwindow *window, int key, int scancode, int action, int mods) {
    global::common_key_callback(window, key, scancode, action, mods);
  };
  ImGuiConnector::Init(global::main_window, primary_monitor, font_scale, key_callback);

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