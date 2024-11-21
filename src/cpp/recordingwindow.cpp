#include <fstream>
#include <fmt/ostream.h>

#include "recordingwindow.h"
#include "globals.h"
#include "keybindings.h"
#include "prm.h"
#include "utils/utils.h"

namespace {
  SharedRecordingPtr rec_from_window_ptr(GLFWwindow *_window) {
    return *std::find_if(prm::recordings.begin(), prm::recordings.end(),
                         [_window](const auto &r) { return r->window == _window; });
  }

  Shader create_frame_shader() {
    return Shader::create(get_rc_text_file("src/shaders/frame.vert.glsl"),
                          get_rc_text_file("src/shaders/frame.frag.glsl"));
  }

  std::tuple<GLuint, GLuint, GLuint> create_frame_vaovboebo() {
    GLuint vao, vbo, ebo;
    // Create Vertex Array Object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create a Vertex Buffer Object and copy the vertex data to it
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLfloat vertices[] = {
        //  Position  Texcoords
        -1.f, 1.f,  0.0f, 0.0f,  // Top-left
        1.f,  1.f,  1.0f, 0.0f,  // Top-right
        1.f,  -1.f, 1.0f, 1.0f,  // Bottom-right
        -1.f, -1.f, 0.0f, 1.0f   // Bottom-left
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create an element array
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    GLuint indices[] = {
        0, 1, 2,  // first triangle
        2, 3, 0   // second triangle
    };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    // texcoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          (void *)(2 * sizeof(GLfloat)));
    return {vao, vbo, ebo};
  }

  Shader create_trace_shader() {
    return Shader::create(get_rc_text_file("src/shaders/trace.vert.glsl"),
                          get_rc_text_file("src/shaders/trace.frag.glsl"),
                          get_rc_text_file("src/shaders/trace.geom.glsl"));
  }

  std::pair<GLuint, GLuint> create_trace_vaovbo() {
    GLuint vao, vbo;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);
    return {vao, vbo};
  }

  Shader create_points_shader() {
    return Shader::create(get_rc_text_file("src/shaders/points.vert.glsl"),
                          get_rc_text_file("src/shaders/points.frag.glsl"));
  }

  std::pair<GLuint, GLuint> create_points_vaovbo() {
    GLuint vao, vbo;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
    return {vao, vbo};
  }
}  // namespace

RecordingWindow::RecordingWindow(std::shared_ptr<AbstractFile> file_,
                                 Transformations transformation_)
    : Recording(std::move(file_)), playback(good() ? length() : 0) {
  if (!good()) {
    return;
  }

  if (Trace::width() == 0) {
    // if unset, set trace edge length to something reasonable
    auto val = std::min(Nx(), Ny()) / 20;
    Trace::width(val);
  }

  if (_file->bitrange()) {
    bitrange = _file->bitrange().value();
  }

  if (!prm::auto_brightness && bitrange != BitRange::NONE) {
    std::tie(get_min(true), get_max(true)) = utils::bitrange_to_float(bitrange);
  } else {
    std::tie(get_min(true), get_max(true)) = oportunistic_minmax(_file, 25);
  }
  if (_file->vmin() && !std::isnan(_file->vmin().value())) {
    get_min(true) = _file->vmin().value();
  }
  if (_file->vmax() && !std::isnan(_file->vmax().value())) {
    get_max(true) = _file->vmax().value();
  }

  if (_file->cmap()) {
    cmap_ = _file->cmap().value();
  } else if (bitrange == BitRange::PHASE || bitrange == BitRange::PHASE_DIFF) {
    // assume that's its a phase map and the user prefers HSV in this circumstances
    cmap_ = ColorMap::HSV;
  }
  if (_file->opacity()) {
    opacity = _file->opacity().value();
  }

  set_transformation(transformation_);
}

void RecordingWindow::open_window() {
  if (window) {
    throw std::runtime_error("ERROR: window was already initialized");
  }

  auto title = name();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  window = glfwCreateWindow(Nx(), Ny(), title.c_str(), nullptr, nullptr);
  if (!window) {
    global::new_ui_message("ERROR: window created failed for {}", title);
    return;
  }

  auto *prev_window = glfwGetCurrentContext();
  glfwMakeContextCurrent(window);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize OpenGL loader!\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  // we don't need to wait for the next frame here, because the main window
  // limits us to 60fps already
  glfwSwapInterval(0);

  add_window_icon(window);
  glfwSetWindowCloseCallback(window, RecordingWindow::close_callback);
  glfwSetKeyCallback(window, RecordingWindow::key_callback);
  glfwSetWindowSizeCallback(window, RecordingWindow::reshape_callback);
  glfwSetCursorPosCallback(window, RecordingWindow::cursor_position_callback);
  glfwSetMouseButtonCallback(window, RecordingWindow::mouse_button_callback);
  glfwSetScrollCallback(window, RecordingWindow::scroll_callback);
  glfwSetWindowAspectRatio(window, Nx(), Ny());
  glfwRequestWindowAttention(window);

  resize_window();
  set_context(window);

  glfwMakeContextCurrent(prev_window);
}

void RecordingWindow::set_context(GLFWwindow *new_context) {
  auto *prev_window = glfwGetCurrentContext();
  if (new_context == nullptr) new_context = window;
  if (window && window != new_context) glfwHideWindow(window);
  if (glcontext && frame_shader) clear_gl_memory();

  glcontext = new_context;
  glfwMakeContextCurrent(glcontext);
  frame_shader                              = create_frame_shader();
  std::tie(frame_vao, frame_vbo, frame_ebo) = create_frame_vaovboebo();
  trace_shader                              = create_trace_shader();
  std::tie(trace_vao, trace_vbo)            = create_trace_vaovbo();
  points_shader                             = create_points_shader();
  std::tie(points_vao, points_vbo)          = create_points_vaovbo();

  update_gl_texture();
  ColorMap cmap_tmp = cmap_;
  std::swap(ctexture, ctexturediff);
  colormap(ColorMap::PRGn);
  std::swap(ctexture, ctexturediff);
  colormap(cmap_tmp);
  frame_shader.use();
  frame_shader.setInt("texture0", 0);
  frame_shader.setInt("textureC", 1);
  checkGlError("init");

  glfwMakeContextCurrent(prev_window);
}

void RecordingWindow::update_gl_texture() {
  if (!glcontext) return;
  auto *prev_window = glfwGetCurrentContext();
  glfwMakeContextCurrent(glcontext);
  if (texture) {
    glDeleteTextures(1, &texture);
  }

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, Nx(), Ny(), 0, GL_RED, GL_FLOAT, frame.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // GL_LINEAR
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // GL_LINEAR
  glfwMakeContextCurrent(prev_window);
}

void RecordingWindow::colormap(ColorMap cmap) {
  if (!glcontext) return;
  auto *prev_window = glfwGetCurrentContext();
  glfwMakeContextCurrent(glcontext);
  if (ctexture) {
    glDeleteTextures(1, &ctexture);
  }

  cmap_ = cmap;
  glGenTextures(1, &ctexture);
  glBindTexture(GL_TEXTURE_1D, ctexture);
  auto cdata = get_colormapdata(cmap);
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F, cdata.size() / 3, 0, GL_RGB, GL_FLOAT, cdata.data());
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // GL_LINEAR
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // GL_LINEAR
  glfwMakeContextCurrent(prev_window);
}

void RecordingWindow::clear_gl_memory() {
  if (!glcontext) return;
  auto *prev_window = glfwGetCurrentContext();
  glfwMakeContextCurrent(glcontext);
  if (texture) {
    glDeleteTextures(1, &texture);
    texture = GL_FALSE;
    glDeleteTextures(1, &ctexture);
    ctexture = GL_FALSE;
    glDeleteTextures(1, &ctexturediff);
    ctexturediff = GL_FALSE;
    glDeleteVertexArrays(1, &frame_vao);
    glDeleteVertexArrays(1, &trace_vao);
    glDeleteBuffers(1, &frame_vbo);
    glDeleteBuffers(1, &frame_ebo);
    glDeleteBuffers(1, &trace_vbo);
    frame_shader.remove();
    trace_shader.remove();
  }
  glfwMakeContextCurrent(prev_window);
}

void RecordingWindow::display() {
  if (!glcontext) throw std::runtime_error("No window set, but RecordingWindow::display() called");

  load_next_frame();

  Eigen::MatrixXf *arr = &frame;
  if (transformation != Transformations::None) {
    assert(transform_ptr);
    transform_ptr->compute(frame, t_frame);
    arr = &transform_ptr->frame;
  }

  switch (transformation) {
    case Transformations::FrameDiff: {
      auto *frameDiff = dynamic_cast<Transformation::FrameDiff *>(transform_ptr.get());
      assert(frameDiff);

      histogram.min = std::min(frameDiff->hist_min, get_min());
      histogram.max = std::max(frameDiff->hist_max, get_max());
    } break;
    case Transformations::ContrastEnhancement:
      histogram.min = 0;
      histogram.max = 1;
      break;
    default:  // None, Gauss, Mean, Median
      if (bitrange != BitRange::NONE) {
        std::tie(histogram.min, histogram.max) = utils::bitrange_to_float(bitrange);
        if (histogram.min < get_min() || histogram.max > get_max()) {
          bitrange = BitRange::NONE;
        }
      }

      if (bitrange == BitRange::NONE) {
        auto [min, max] = utils::minmax_element_skipNaN(arr->data(), arr->data() + arr->size());
        if (min < histogram.min) {
          histogram.min = min;
        }
        if (max > histogram.max) {
          histogram.max = max;
        }
      }
      break;
  }

  histogram.compute(arr->reshaped());

  // update traces
  if (playback.next_t() != playback.current_t()) {
    for (auto &trace : traces) {
      auto [start, size] = Trace::clamp(trace.pos, {Nx(), Ny()});
      if (size[0] > 0 && size[1] > 0) {
        auto block = arr->block(start[0], start[1], size[0], size[1]);
        trace.data.push_back(block.mean());
        //block.setConstant(0);  // for testing
      }
    }
  }

  /* render code */
  glfwMakeContextCurrent(window);
  glClear(GL_COLOR_BUFFER_BIT);
  glfwMakeContextCurrent(glcontext);

  frame_shader.use();
  frame_shader.setVec2("minmax", get_min(), get_max());
  frame_shader.setInt("opacity_function", static_cast<int>(opacity));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Nx(), Ny(), GL_RED, GL_FLOAT, arr->data());
  glActiveTexture(GL_TEXTURE1);
  GLuint color_tex = ctexture;
  if (transformation == Transformations::FrameDiff && !is_diff_colormap(cmap_)) {
    color_tex = ctexturediff;
  }
  glBindTexture(GL_TEXTURE_1D, color_tex);

  // Draw the frame
  glBindVertexArray(frame_vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  if (!children.empty()) {
    for (const auto &crec : children) {
      if (crec->active) crec->display();
    }
    glfwMakeContextCurrent(window);
    //    glClear(GL_COLOR_BUFFER_BIT);
    glfwMakeContextCurrent(glcontext);
  }

  // Draw the traces rectangle
  if (!traces.empty()) {
    trace_vert.clear();
    auto coordtrans = [this](const Vec2i &pos) {
      int isodd = Trace::width() % 2;
      float x   = (2.f * pos[0] + isodd) / static_cast<float>(Nx()) - 1;
      float y   = 1 - (2.f * pos[1] + isodd) / static_cast<float>(Ny());
      trace_vert.push_back(x);
      trace_vert.push_back(y);
    };
    for (auto &trace : traces) {
      coordtrans(trace.pos);
      trace_vert.push_back(trace.color[0]);
      trace_vert.push_back(trace.color[1]);
      trace_vert.push_back(trace.color[2]);
    }

    trace_shader.use();
    trace_shader.setVec2("halfwidth", Trace::width() / static_cast<float>(Nx()),
                         Trace::width() / static_cast<float>(Ny()));
    glBindVertexArray(trace_vao);
    glBindBuffer(GL_ARRAY_BUFFER, trace_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * trace_vert.size(), trace_vert.data(),
                 GL_STREAM_DRAW);
    glLineWidth(1.5);
    glDrawArrays(GL_POINTS, 0, traces.size());
  }

  for (const auto &flow : flows) {
    if (!flow.show) continue;
    points_vert.clear();

    flow.data->load_frame(2 * t_frame);
    Eigen::MatrixXf u = flow.data->frame;  // force a copy
    flow.data->load_frame(2 * t_frame + 1);
    auto v  = flow.data->frame;  // don't need to force a copy
    auto nx = flow.data->Nx(), ny = flow.data->Ny();
    // get flow signs based on current rotation and flip
    auto [signx, signy] = rotations.flow_signs();
    for (int x = FlowData::skip / 2; x < nx; x += FlowData::skip) {
      for (int y = FlowData::skip / 2; y < ny; y += FlowData::skip) {
        // calculate screen position of the pixel center
        float xx = (2.f * x + 1) / static_cast<float>(nx) - 1;
        float yy = 1 - (2.f * y + 1) / static_cast<float>(ny);
        // add the flow
        xx += signx * 2.f * u(x, y) / static_cast<float>(nx);
        yy -= signy * 2.f * v(x, y) / static_cast<float>(ny);

        points_vert.push_back(xx);
        points_vert.push_back(yy);
      }
    }

    glEnable(GL_PROGRAM_POINT_SIZE);
    points_shader.use();
    points_shader.setVec4("color", flow.color);
    points_shader.setFloat("pointsize", FlowData::pointsize * scale_fct);
    glBindVertexArray(points_vao);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * points_vert.size(), points_vert.data(),
                 GL_STREAM_DRAW);
    glDrawArrays(GL_POINTS, 0, points_vert.size() / 2);
  }

  for (const auto &vid : points_videos) {
    if (!vid->show) continue;
    points_vert.clear();

    if (vid->point_size < 0) {
      vid->point_size = FlowData::pointsize;
    }

    auto data = vid->data.at(t_frame);
    float nx = Nx(), ny = Ny();
    for (size_t i = 0; i < data.size(); i += 2) {
      const float x = (2.f * data[i + 1] + 1) / nx - 1;
      const float y = 1 - (2.f * data[i] + 1) / ny;
      points_vert.push_back(x);
      points_vert.push_back(y);
    }

    glEnable(GL_PROGRAM_POINT_SIZE);
    points_shader.use();
    points_shader.setVec4("color", vid->color);
    points_shader.setFloat("pointsize", vid->point_size * scale_fct);
    glBindVertexArray(points_vao);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * points_vert.size(), points_vert.data(),
                 GL_STREAM_DRAW);
    glDrawArrays(GL_POINTS, 0, points_vert.size() / 2);
  }

  glfwMakeContextCurrent(prm::main_window);
}

void RecordingWindow::set_name(const std::string &new_name) {
  if (window) {
    glfwSetWindowTitle(window, new_name.c_str());
  }
  Recording::set_name(new_name);
}

void RecordingWindow::reset_traces() {
  for (auto &t : traces) {
    t.clear();
  }
}
void RecordingWindow::add_trace(const Vec2i &pos) {
  traces.push_back({std::rand(), pos, Trace::next_color()});
}
void RecordingWindow::remove_trace(const Vec2i &pos) {
  const auto pred = [pos](const auto &trace) { return trace.is_near_point(pos); };

  traces.erase(std::remove_if(traces.begin(), traces.end(), pred), traces.end());
}
void RecordingWindow::save_trace(const Vec2i &pos, fs::path path, Vec2i t0tmax) {
  if (t0tmax[0] == t0tmax[1]) {
    t0tmax = {0, length()};
  }
  if (t0tmax[0] < 0 || t0tmax[1] > length() || t0tmax[0] > t0tmax[1]) {
    global::new_ui_message("ERROR: start or end frame invalid, start frame {}, end frame {}",
                           t0tmax[0], t0tmax[1]);
  }
  auto cur_frame = t_frame;

  auto [start, size] = Trace::clamp(pos, {Nx(), Ny()});
  if (size[0] <= 0 && size[1] <= 0) {
    global::new_ui_message("Failed to save trace, trace size is invalid ({}, {})", size[0], size[1]);
    return;
  }

  fs::remove(path);
  std::ofstream file(path.string(), std::ios::out);
  fmt::print(file, "Frame\tValue\n");

  for (int t = t0tmax[0]; t < t0tmax[1]; t++) {
    load_frame(t);
    auto block = frame.block(start[0], start[1], size[0], size[1]);
    fmt::print(file, "{}\t{}\n", t, block.mean());
  }

  fmt::print("Saved trace to {}\n", path.string());
  load_frame(cur_frame);
}

void RecordingWindow::resize_window() {
  int width  = std::ceil(RecordingWindow::scale_fct * Nx());
  int height = std::ceil(RecordingWindow::scale_fct * Ny());

  glfwSetWindowSize(window, width, height);
  reshape_callback(window, width, height);
}

fs::path RecordingWindow::save_snapshot(std::string output_png_path_template) {
  if (output_png_path_template.empty()) {
    auto stem                = path().stem().string();
    output_png_path_template = stem.empty() ? "{t}.png" : stem + "_{t}.png";
  }
  auto out_path = fmt::format(output_png_path_template, fmt::arg("t", t_frame));

  gl_save_snapshot(out_path, window);
  return out_path;
}

void RecordingWindow::scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  // if traces shown, change trace width, else window width
  auto rec = rec_from_window_ptr(window);
  if (!rec->traces.empty()) {
    auto w    = Trace::width();
    int new_w = (yoffset < 0) ? 0.95f * w : 1.05f * w;
    if (new_w == w) {
      new_w = (yoffset < 0) ? w - 1 : w + 1;
    }
    Trace::width(new_w);
  } else {
    scale_fct = (yoffset < 0) ? 0.95f * scale_fct : 1.05f * scale_fct;
    for (const auto &r : prm::recordings) {
      r->resize_window();
    }
  }
}

void RecordingWindow::cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
  auto rec      = rec_from_window_ptr(window);
  rec->mousepos = {xpos, ypos};
  if (rec->mousebutton.holding_left) {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    int x     = rec->mousepos[0] * rec->Nx() / w;
    int y     = rec->mousepos[1] * rec->Ny() / h;
    Vec2i pos = {x, y};

    // if any trace point is nearby, move it to the new position
    for (auto &t : rec->traces) {
      if (t.is_near_point(pos)) {
        t.set_pos(pos);
        return;
      }
    }

    // only add trace points on mouse press
    if (rec->mousebutton.pressing_left) {
      rec->add_trace({x, y});
      rec->mousebutton.pressing_left = false;
    }
  }
}

void RecordingWindow::mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  auto rec = rec_from_window_ptr(window);
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      rec->mousebutton.holding_left  = true;
      rec->mousebutton.pressing_left = true;
      rec->cursor_position_callback(window, rec->mousepos[0], rec->mousepos[1]);
    } else {
      rec->mousebutton.holding_left  = false;
      rec->mousebutton.pressing_left = false;
    }
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      int w, h;
      glfwGetWindowSize(window, &w, &h);
      int x = rec->mousepos[0] * rec->Nx() / w;
      int y = rec->mousepos[1] * rec->Ny() / h;

      rec->remove_trace({x, y});
    }
  }
}

void RecordingWindow::reshape_callback(GLFWwindow *window, int w, int h) {
  auto rec = rec_from_window_ptr(window);

  if (!rec) {
    throw std::runtime_error(
        "Error in RecordingWindow::reshape_callback, "
        "could not find associated recording");
  }

  glfwMakeContextCurrent(window);
  glViewport(0, 0, w, h);
  glClearColor(1.f, 1.f, 1.f, 0.f);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);

  glfwMakeContextCurrent(prm::main_window);
}

void RecordingWindow::close_callback(GLFWwindow *window) {
  prm::recordings.erase(std::remove_if(prm::recordings.begin(), prm::recordings.end(),
                                       [window](auto r) { return r->window == window; }),
                        prm::recordings.end());
}

void RecordingWindow::key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  global::common_key_callback(window, key, scancode, action, mods, rec_from_window_ptr(window));
}

void RecordingWindow::start_recording(const fs::path &filename, int fps, std::string description) {
  playback.set_next(export_ctrl.video.tstart);
  for (auto &child : children) {
    child->playback.set_next(export_ctrl.video.tstart);
  }
  prm::playbackCtrl.play();
  if (export_ctrl.video.videoRecorder.start_recording(filename.string(), window, fps, description)) {
    export_ctrl.video.recording = true;
    export_ctrl.video.progress  = 0;
    prm::lastframetime          = std::numeric_limits<float>::lowest();
  }
}

void RecordingWindow::stop_recording() {
  export_ctrl.video.videoRecorder.stop_recording();
  export_ctrl.video.recording     = false;
  export_ctrl.video.progress      = 0;
  export_ctrl.video.export_window = false;
  prm::lastframetime              = glfwGetTime();
}

void RecordingWindow::set_rotation(short rotation) {
  rotations.set_rotation(rotation);
  for (const auto &rec : prm::recordings) {
    rec->rotation_was_changed();
  }
}
void RecordingWindow::add_rotation(short d_rotation) {
  rotations.add_rotation(d_rotation);
  for (const auto &rec : prm::recordings) {
    rec->rotation_was_changed();
  }
}
void RecordingWindow::rotation_was_changed() {
  load_frame(t_frame);
  if (transform_ptr) {
    transform_ptr->allocate(*this);
  }
  resize_window();
  update_gl_texture();
  glfwSetWindowAspectRatio(window, Nx(), Ny());
  traces.clear();
}

void RecordingWindow::add_flow(std::shared_ptr<Recording> flow) {
  auto inmemory = dynamic_cast<InMemoryFile *>(flow->file().get());
  if (inmemory) {
    auto color = inmemory->color();
    if (color.has_value()) {
      flows.emplace_back(flow, color.value());
      return;
    }
  }

  if (flow->length() >= 2 * length())
    flows.emplace_back(flow, flows.size());
  else
    global::new_ui_message("Failed to add '{}' as flow to '{}' because length does not match",
                           flow->name(), name());
}

void RecordingWindow::add_points_video(std::shared_ptr<global::PointsVideo> pv) {
  if (!pv) return;
  if (pv->data.size() != length()) {
    global::new_ui_message(
        "Failed to add points to recording, number of frames do not match (recording {}, points {}",
        length(), pv->data.size());
    return;
  }
  if (pv->color[3] == 0) {
    pv->assign_next_color(points_videos.size());
  }
  points_videos.push_back(pv);
}

void RecordingWindow::render() {
  if (glcontext == window) {
    glfwMakeContextCurrent(window);

    if (export_ctrl.video.recording) {
      if (playback.current_t() < 0) {
        // do nothing, this is the frame before the first one
      } else if (playback.is_last() || playback.current_t() > export_ctrl.video.tend) {
        stop_recording();
        global::new_ui_message("Exporting video finished!");
      } else {
        export_ctrl.video.videoRecorder.add_frame();
        export_ctrl.video.progress = playback.progress();
      }
    }

    glfwSwapBuffers(window);

    glfwMakeContextCurrent(prm::main_window);
  }
}

void RecordingWindow::set_transformation(Transformations type) {
  transform_ptr  = Transformation::factory(type, *this);
  transformation = type;
}
