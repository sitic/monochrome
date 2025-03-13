#pragma once

#ifdef _WIN32  // Windows 32 and 64 bit
#include <windows.h>
#endif

#include <cstdlib>
#include <vector>
#include <thread>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "utils/ImGuiConnector.h"
#include "imspinner.h"

#include "globals.h"
#include "prm.h"
#include "recordingwindow_rgb.h"
#include "recordingwindow.h"
#include "ui.h"
#include "keybindings.h"
#include "utils/files.h"
#include "utils/settings.h"

SharedRecordingPtr find_parent_recording(const std::string &parent_name) {
  // If the parent_name is empty we default to using the last loaded video
  if (parent_name.empty() && !prm::recordings.empty()) return prm::recordings.back();

  // Find the last video with the name `parent_name`
  auto it = std::find_if(prm::recordings.rbegin(), prm::recordings.rend(),
                         [name = parent_name](const auto &r) { return name == r->name(); });
  if (it == std::rend(prm::recordings))
    return SharedRecordingPtr(nullptr);
  else
    return *it;
}

void load_new_file(std::shared_ptr<AbstractFile> file,
                   std::optional<std::string> parentName = std::nullopt) {
  if (!file || !file->good()) return;

  if (file->Nc() != 2) {  // Regular video
    SharedRecordingPtr rec;
    rec = file->Nc() == 1? std::make_shared<RecordingWindow>(file): std::make_shared<RGBRecordingWindow>(file);
                         

    static int video_counter = 0;
    video_counter++;
    if (rec->name().empty()) {
      rec->set_name(fmt::format("Video {}", video_counter));
    }

    if (!prm::recordings.empty()) {
      rec->playback.synchronize_with(prm::recordings.back()->playback, false);
    }

    if (parentName) {
      auto parent = find_parent_recording(parentName.value());
      if (!parent) {
        if (prm::recordings.empty())
          global::new_ui_message("ERROR: You need to load a video first before adding a layer");
        else
          global::new_ui_message(
              "ERROR: Video \"{}\" has requested \"{}\" as its parent recording, but no such "
              "recording exists!",
              rec->name(), parentName.value());
        return;
      }
      prm::merge_queue.emplace(rec, parent);
    } else { // Normal media, no parent

      // If the video/image is very large or very small, scale it to something reasonable
      // Only do this if it's the first video and no scaling has been set yet
      if (prm::recordings.empty() && RecordingWindow::scale_fct == 1.0f) {
        auto primary_monitor = glfwGetPrimaryMonitor();
        int x, y, width, height;
        glfwGetMonitorWorkarea(primary_monitor, &x, &y, &width, &height);
        auto mode    = glfwGetVideoMode(primary_monitor);
        auto scale_x = width / static_cast<float>(rec->Nx());
        auto scale_y = height / static_cast<float>(rec->Ny());
        if (scale_x < 1 || scale_y < 1) {
          // Media is very large, scale it down
          RecordingWindow::scale_fct = std::min(scale_x * 0.9f, scale_y * 0.9f);
        } else if (scale_x > 10 || scale_y > 10) {
          // Media is very small, scale it up
          RecordingWindow::scale_fct = std::min(scale_x * .5f, scale_y * .5f);
        }
      }

      // Finally, open the recording
      prm::recordings.push_back(rec);
      settings::add_recent_file(rec->filepath());
      rec->open_window();
    }
  } else {  // Flow array
    SharedRecordingPtr parent;
    if (!parentName) {
      parentName = "";
    }
    parent = find_parent_recording(parentName.value());
    if (!parent) {
      std::string err_msg =
          prm::recordings.empty()
              ? "ERROR: Failed to add flow to recording, no recording with name \"{}\" exists!"
              : "ERROR: Failed to add flow to recording, you need to load a recording first!";
      global::new_ui_message(err_msg, parentName.value());
      return;
    }

    auto rec = std::make_shared<Recording>(file);

    static int flow_counter = 0;
    flow_counter++;
    if (rec->name().empty()) {
      rec->set_name(fmt::format("Flow {}", flow_counter));
    }

    parent->add_flow(rec);
  }
}

void load_new_file(const fs::path &path) {
  fmt::print("Loading {} ...\n", path.string());
  load_new_file(file_factory(path));
}

void load_new_pointsvideo(std::shared_ptr<global::PointsVideo> pointsvideo) {
  if (prm::recordings.empty()) {
    global::new_ui_message("ERROR: A video needs to be opened before points lists can be displayed");
    return;
  }
  auto parent_name = pointsvideo->parent_name;
  auto parent      = find_parent_recording(parent_name);
  if (!parent) {
    global::new_ui_message(
        "ERROR: Points Array \"{}\" has requested \"{}\" as its parent recording, but no such "
        "recording exists!",
        pointsvideo->name, parent_name);
    return;
  }

  static int points_video_counter = 0;
  points_video_counter++;
  if (pointsvideo->name.empty()) {
    pointsvideo->name = fmt::format("PointsVideo {}", points_video_counter);
  }
  parent->add_points_video(pointsvideo);
}

/* Check all our queues for new elements */
void load_from_queue() {
  /* Remote Commands */
  if (auto command = global::get_remote_command(); command && command.value()) {
    std::shared_ptr<global::RemoteCommand> cmd = command.value();
    if (auto obj = std::dynamic_pointer_cast<global::LoadFileCommand>(cmd)) { // Load file
      load_new_file(obj->filename);
    } else if (auto obj = std::dynamic_pointer_cast<global::RawArray3>(cmd)) { // Load InMemory array
      load_new_file(std::make_shared<InMemoryFile>(obj), obj->meta.parentName);
    } else if (auto obj = std::dynamic_pointer_cast<global::PointsVideo>(cmd)) { // Load PointsVideo
      load_new_pointsvideo(obj);
    } else if (auto obj = std::dynamic_pointer_cast<global::ExportVideoCommand>(cmd)) { // Export video
      if (auto rec = find_parent_recording(obj->recording)) {
        rec->start_recording(*obj);
      } else {
        global::new_ui_message("ERROR: Export requested for recording \"{}\", but no such recording exists!",
                               obj->recording);
      }
    } else if (auto obj = std::dynamic_pointer_cast<global::CloseVideoCommand>(cmd)) { // Close video
      if (auto rec = find_parent_recording(obj->recording)) {
        glfwSetWindowShouldClose(rec->window, GLFW_TRUE);
      } else {
        global::new_ui_message("ERROR: Close requested for recording \"{}\", but no such recording exists!", obj->recording);
      }
    } else {
      global::new_ui_message("ERROR: Unknown remote command");
    }
  }

  /* Global merge queue */
  if (!prm::merge_queue.empty()) {
    auto [child, parent] = prm::merge_queue.front();
    prm::merge_queue.pop();
    parent->children.push_back(child);
    child->set_context(parent->window);
    child->playback = parent->playback;
    prm::recordings.erase(
        std::remove_if(prm::recordings.begin(), prm::recordings.end(),
                       [ptr = child.get()](const auto &r) -> bool { return r.get() == ptr; }),
        prm::recordings.end());
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

void show_install_uv_dialog() {
  bool installation_in_progress = utils::uv_install_in_progress();
  
  ImGui::SetNextWindowSizeConstraints(ImVec2(400, 0), ImVec2(FLT_MAX, FLT_MAX));
  ImGui::Begin("Extra Dependencies Required", &installation_in_progress, ImGuiWindowFlags_AlwaysAutoResize);
  const std::string message = \
R"(Monochrome requires [`uv`](https://github.com/astral-sh/uv), a fast Python package manager written in Rust, to load certain file formats through plugins.

This is a one-time installation that will enable you to work with more file types. `uv` is lightweight, secure, and can be easily [uninstalled](https://docs.astral.sh/uv/getting-started/installation/#uninstallation) later if needed. [Learn more about `uv`.](https://docs.astral.sh/uv/)

Continue with automatic installation of `uv`?)";
  static ImGuiConnector::markdown md;
  md.print(message.data(), message.data() + message.size());
  
  if (ImGui::Button("Yes", ImVec2(120, 0))) {
    utils::install_uv();
  }
  ImGui::SameLine();
  if (ImGui::Button("No", ImVec2(120, 0))) {
    global::subprocesses.clear();
  }

  ImGui::End();
}

void show_subprocesses() {
  // Check if subprocesses should be cleared
  global::subprocesses.erase(
      std::remove_if(global::subprocesses.begin(), global::subprocesses.end(),
                     [](const auto &p) -> bool { return !p->show && !p->is_running(); }),
                             global::subprocesses.end());

  // Do we need to install UV?
  if (std::any_of(global::subprocesses.begin(), global::subprocesses.end(),
                [](const auto &p) -> bool { return !p->open(); })) {
    show_install_uv_dialog();
  }

  // Show subprocesses
  for (auto &p : global::subprocesses) {
    if (!p->tick()) continue;
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.5f * ImGui::GetMainViewport()->Size[0], 0),
                                         ImVec2(FLT_MAX, FLT_MAX));
    std::string window_title = fmt::format("{}##{}", p->title, p->id);
    ImGui::Begin(window_title.c_str(), &(p->show));

    ImGui::TextWrapped("%s", p->msg.c_str());

    if (!p->is_running()) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
      ImGui::PushFont(ImGuiConnector::font_bold);
      ImGui::TextWrapped("Error encountered during file loading process!");
      ImGui::PopFont();
      ImGui::PopStyleColor();
    }
  
    ImGuiTreeNodeFlags header_flags = ImGuiTreeNodeFlags_DefaultOpen;
    bool is_open = ImGui::CollapsingHeader("Plugin Output", header_flags);
    if (is_open) {
      ImGui::BeginChild("subprocess_output", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);
      if (!p->is_running()) {
        ImGui::TextUnformatted(p->cmd.c_str());
        ImGui::Separator();
      }
      ImGui::TextUnformatted(p->cout.c_str());
      if (p->is_running()) ImSpinner::SpinnerFadeDots("SpinnerBounceBall", 16, 2, ImSpinner::white, 5.66);
      if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
      ImGui::EndChild();
    }

    if (!p->is_running()) {
      if (ImGui::Button("Ok", ImVec2(-1.0f, 0.0f))) {
        p->show = false;
      }
    }
    ImGui::End();
  }
}

void display_loop() {
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  prm::lastframetime = glfwGetTime();
  // keep running until main window is closed
  while (!glfwWindowShouldClose(prm::main_window)) {
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

    // Load new files from the queue
    load_from_queue();

    // Poll and handle events
    glfwPollEvents();

    // Check if recording window should close
    prm::recordings.erase(std::remove_if(prm::recordings.begin(), prm::recordings.end(),
                                         [](const auto &r) -> bool {
                                           return r->window && glfwWindowShouldClose(r->window);
                                         }),
                          prm::recordings.end());

    // Show ImGui windows
    ImGuiConnector::NewFrame();
    show_main_imgui_window();
    show_messages();
    show_subprocesses();

    // ImSpinner::demoSpinners();
    // ImGui::ShowDemoWindow();

    // Rendering
    for (const auto &recording : prm::recordings) {
      recording->render();
    }
    ImGuiConnector::Render(prm::main_window, clear_color);
    glfwSwapBuffers(prm::main_window);
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
  prm::main_window = glfwCreateWindow(prm::main_window_width, prm::main_window_height, "Monochrome",
                                      nullptr, nullptr);
  if (!prm::main_window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwGetFramebufferSize(prm::main_window, &prm::main_window_width, &prm::main_window_height);
  glfwMakeContextCurrent(prm::main_window);
  // wait until the current frame has been drawn before drawing the next one
  glfwSwapInterval(0);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize OpenGL loader!\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  add_window_icon(prm::main_window);
  // On retina displays, the framebuffer size is twice the window size
  glfwGetWindowSize(prm::main_window, &prm::main_window_width, &prm::main_window_height);

  // Initialize Callbacks
  glfwSetWindowSizeCallback(prm::main_window, [](GLFWwindow *window, int w, int h) {
    prm::main_window_width  = w;
    prm::main_window_height = h;
  });
  glfwSetDropCallback(prm::main_window, [](GLFWwindow *window, int count, const char **paths) {
    for (int i = 0; i < count; i++) {
      load_new_file(paths[i]);
    }
  });
  auto key_callback = [](GLFWwindow *window, int key, int scancode, int action, int mods) {
    global::common_key_callback(window, key, scancode, action, mods);
  };

  ImGuiConnector::Init(prm::main_window, primary_monitor, font_scale, key_callback);

  // Initialize colormap textures
  int num_cmaps = sizeof(ColorMapsNames) / sizeof(ColorMapsNames[0]);
  for (int i = 0; i < num_cmaps; i++) {
    ColorMap cmap = static_cast<ColorMap>(i);
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