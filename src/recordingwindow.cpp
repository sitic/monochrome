#include "recordingwindow.h"

namespace global {
  extern GLFWwindow *main_window;
  extern std::vector<std::shared_ptr<RecordingWindow>> recordings;
}  // namespace global

float RecordingWindow::scale_fct = 1;

namespace {
  std::shared_ptr<RecordingWindow> from_window_ptr(GLFWwindow *_window) {
    return *std::find_if(global::recordings.begin(), global::recordings.end(),
                         [_window](const auto &r) { return r->window == _window; });
  }
}  // namespace

std::pair<int, float> RecordingPlaybackCtrl::next_timestep(float speed_) const {
  auto tf = tf_ + speed_;
  auto t  = tf_;

  while (tf > length_ - 1) {
    tf -= length_ - 1;
    t = 0;
  }

  if (std::floor(tf_) > t_) {
    t = std::floor(tf_);
  }

  if (t < 0) {
    // should never happen, but just in case
    t  = 0;
    tf = 0;
  }
  return {t, tf};
}

int RecordingPlaybackCtrl::step() {
  std::tie(t_, tf_) = next_timestep(prm::playbackCtrl.val);
  return t_;
}

int RecordingPlaybackCtrl::next_t() const {
  return next_timestep(prm::playbackCtrl.val).first;
}
int RecordingPlaybackCtrl::next_t(int iterations) const {
  return next_timestep(iterations * prm::playbackCtrl.val).first;
}
float RecordingPlaybackCtrl::progress() const {
  return t_ / static_cast<float>(length_ - 1);
}
void RecordingPlaybackCtrl::set(int t) {
  t_  = t;
  tf_ = t;
}
void RecordingPlaybackCtrl::restart() {
  t_  = std::numeric_limits<int>::lowest();
  tf_ = std::numeric_limits<float>::lowest();
}

void Trace::set_pos(const Vec2i &npos) {
  clear();
  pos = npos;
}

bool Trace::is_near_point(const Vec2i &npos) const {
  const auto d        = npos - pos;
  const auto max_dist = Trace::width() / 2 + 1;
  return (std::abs(d[0]) < max_dist && std::abs(d[1]) < max_dist);
}

Vec4f Trace::next_color() {
  // List of colors to cycle through
  static std::array<Vec4f, 4> cycle_list = {{
      {228 / 255.f, 26 / 255.f, 28 / 255.f, 1},
      {55 / 255.f, 126 / 255.f, 184 / 255.f, 1},
      {77 / 255.f, 175 / 255.f, 74 / 255.f, 1},
      {152 / 255.f, 78 / 255.f, 163 / 255.f, 1},
  }};

  static int count = -1;
  count++;
  if (count >= cycle_list.size()) {
    count = 0;
  }
  return cycle_list.at(count);
}

int Trace::width(int new_width) {
  static int w = 0;

  if (new_width > 0) {
    w = new_width;
    for (auto rec : global::recordings) {
      rec->auto_shrink_traces();
    }
  }

  return w;
}

TransformationList::TransformationCtrl::TransformationCtrl(
    std::variant<Transformations, Filters> type, Recording &rec, int _gen)
    : m_gen(_gen), m_type(type) {
  if (auto t = std::get_if<Transformations>(&type)) {
    switch (*t) {
      case Transformations::None:
        m_transform = std::make_unique<Transformation::None>(rec);
        break;
      case Transformations::FrameDiff:
        m_transform = std::make_unique<Transformation::FrameDiff>(rec);
        break;
      case Transformations::ContrastEnhancement:
        m_transform = std::make_unique<Transformation::ContrastEnhancement>(rec);
        break;
      case Transformations::FlickerSegmentation:
        m_transform = std::make_unique<Transformation::FlickerSegmentation>(rec);
        break;
    }
  } else if (auto t = std::get_if<Filters>(&type)) {
    switch (*t) {
      case Filters::None:
        m_transform = std::make_unique<Transformation::None>(rec);
        break;
      case Filters::Gauss:
        m_transform = std::make_unique<Transformation::GaussFilter>(rec);
        break;
      case Filters::Mean:
        m_transform = std::make_unique<Transformation::MeanFilter>(rec);
        break;
      case Filters::Median:
        m_transform = std::make_unique<Transformation::MedianFilter>(rec);
        break;
    }
  }
}

Transformation::Base *TransformationList::create_if_needed(
    std::variant<Transformations, Filters> type, int gen) {
  auto r = std::find_if(transformations.begin(), transformations.end(),
                        [type, gen](const TransformationCtrl &t) -> bool {
                          return (t.type() == type) && (t.gen() == gen);
                        });
  if (r != std::end(transformations)) {
    return r->transformation();
  } else {
    transformations.emplace_back(type, m_parent, gen);
    return transformations.back().transformation();
  }
}

TransformationList::TransformationList(Recording &rec) : m_parent(rec) {
  if (!rec.good()) return;
  transformations.emplace_back(Transformations::None, m_parent, 0);
  transformations.emplace_back(Filters::None, m_parent, 0);
  transformations.emplace_back(Filters::None, m_parent, 1);
}

void TransformationList::reallocate() {
  for (auto &t : transformations) {
    t.transformation()->allocate(m_parent);
  }
}

void RecordingWindow::scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  // if traces shown, change trace width, else window width
  std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);
  if (!rec->traces.empty()) {
    auto w    = Trace::width();
    int new_w = (yoffset < 0) ? 0.95f * w : 1.05f * w;
    if (new_w == w) {
      new_w = (yoffset < 0) ? w - 1 : w + 1;
    }
    Trace::width(new_w);
  } else {
    scale_fct = (yoffset < 0) ? 0.95f * scale_fct : 1.05f * scale_fct;
    for (const auto &r : global::recordings) {
      r->resize_window();
    }
  }
}

void RecordingWindow::cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
  std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);
  rec->mousepos                        = {xpos, ypos};
  if (rec->mousebutton.left) {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    int x = rec->mousepos[0] * rec->Nx() / w;
    int y = rec->mousepos[1] * rec->Ny() / h;

    rec->add_trace_pos({x, y});
  }
}

void RecordingWindow::mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      rec->mousebutton.left = true;
      rec->cursor_position_callback(window, rec->mousepos[0], rec->mousepos[1]);
    } else {
      rec->mousebutton.left = false;
    }
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      int w, h;
      glfwGetWindowSize(window, &w, &h);
      int x = rec->mousepos[0] * rec->Nx() / w;
      int y = rec->mousepos[1] * rec->Ny() / h;

      rec->remove_trace_pos({x, y});
    }
  }
}

void RecordingWindow::reshape_callback(GLFWwindow *window, int w, int h) {
  std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);

  if (!rec) {
    throw std::runtime_error(
        "Error in RecordingWindow::reshape_callback, "
        "could not find associated recording");
  }

  glfwMakeContextCurrent(window);
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();  // Reset The Projection Matrix
  glOrtho(0, rec->Nx(), 0, rec->Ny(), -1, 1);
  // https://docs.microsoft.com/en-us/previous-versions//ms537249(v=vs.85)?redirectedfrom=MSDN
  // http://www.songho.ca/opengl/gl_projectionmatrix.html#ortho
  glMatrixMode(GL_MODELVIEW);  // Select The Modelview Matrix
  glLoadIdentity();            // Reset The Modelview Matrix

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  glfwMakeContextCurrent(global::main_window);
}

void RecordingWindow::close_callback(GLFWwindow *window) {
  global::recordings.erase(std::remove_if(global::recordings.begin(), global::recordings.end(),
                                          [window](auto r) { return r->window == window; }),
                           global::recordings.end());
}

void RecordingWindow::key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
    // Don't call RecordingWindow::close_callback() directly here,
    // causes a segfault in glfw
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  } else if (key == GLFW_KEY_P && action == GLFW_PRESS) {
    std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);
    auto fn                              = rec->save_snapshot();
    global::new_ui_message("Saved screenshot to {}", fn.string());
  } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    prm::playbackCtrl.toggle_play_pause();
  }
}

void RecordingWindow::open_window() {
  if (window) {
    throw std::runtime_error("ERROR: window was already initialized");
  }

  auto title = path().filename().string();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  window = glfwCreateWindow(Nx(), Ny(), title.c_str(), nullptr, nullptr);
  if (!window) {
    global::new_ui_message("ERROR: window created failed for {}", title);
    return;
  }

  auto prev_window = glfwGetCurrentContext();
  glfwMakeContextCurrent(window);

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

  glfwMakeContextCurrent(prev_window);
}

void RecordingWindow::resize_window() {
  int width  = std::ceil(RecordingWindow::scale_fct * Nx());
  int height = std::ceil(RecordingWindow::scale_fct * Ny());

  glfwSetWindowSize(window, width, height);
  reshape_callback(window, width, height);
}

fs::path RecordingWindow::save_snapshot(std::string output_png_path_template) {
  if (output_png_path_template.empty()) {
    output_png_path_template = path().stem().string() + "_{t}.png";
  }
  auto out_path = fmt::format(output_png_path_template, fmt::arg("t", t_frame));

  gl_save_snapshot(out_path, window);
  return out_path;
}

void RecordingWindow::display(Filters prefilter,
                              Transformations transformation,
                              Filters postfilter,
                              BitRange bitrange) {
  if (!window) throw std::runtime_error("No window set, but RecordingWindow::display() called");

  glfwMakeContextCurrent(window);

  load_next_frame();

  Eigen::MatrixXf *arr = &frame;

  auto pretransform = transformationArena.create_if_needed(prefilter, 0);
  pretransform->compute(*arr, t_frame);
  arr = &pretransform->frame;

  auto transform = transformationArena.create_if_needed(transformation, 0);
  transform->compute(*arr, t_frame);
  arr = &transform->frame;

  switch (transformation) {
    case Transformations::None:
      histogram.min = 0;
      histogram.max = bitrange_to_float(bitrange);
      break;
    case Transformations::FrameDiff: {
      auto frameDiff = dynamic_cast<Transformation::FrameDiff *>(transform);
      assert(frameDiff);

      histogram.min = frameDiff->min_init() * 1.5f;
      histogram.max = frameDiff->max_init() * 1.5f;
    } break;
    case Transformations::ContrastEnhancement:
      histogram.min = -0.1f;
      histogram.max = 1.1f;
      break;
    case Transformations::FlickerSegmentation:
      histogram.min = 0;
      histogram.max = (*arr).maxCoeff();
      break;
  }

  auto posttransform = transformationArena.create_if_needed(postfilter, 0);
  posttransform->compute(*arr, t_frame);
  arr = &posttransform->frame;

  draw2dArray(*arr, vert, vert_color_buffer, get_min(transformation), get_max(transformation));
  histogram.compute(arr->reshaped());

  // draw and update traces
  if (playback.next_t() != playback.current_t()) {
    for (auto &[trace, pos, color] : traces) {
      auto w      = Trace::width();
      Vec2i start = {pos[0] - w / 2, pos[1] - w / 2};
      auto block  = arr->block(start[0], start[1], w, w);
      trace.push_back(block.mean());
      drawPixel(pos[0], pos[1], Ny(), w, color);
    }
  }

  glfwSwapBuffers(window);

  if (export_ctrl.video.recording) {
    auto cur = playback.progress();
    if (cur < export_ctrl.video.progress) {
      stop_recording();
      global::new_ui_message("Exporting video finished!");
    } else {
      export_ctrl.video.videoRecorder.add_frame();
      export_ctrl.video.progress = cur;
    }
  }

  glfwMakeContextCurrent(global::main_window);
}

void RecordingWindow::reset_traces() {
  for (auto &t : traces) {
    t.clear();
  }
}
void RecordingWindow::add_trace_pos(const Vec2i &npos) {
  for (auto &t : traces) {
    if (t.is_near_point(npos)) {
      t.set_pos(npos);
      return;
    }
  }
  traces.push_back({{}, npos, Trace::next_color()});
}
void RecordingWindow::remove_trace_pos(const Vec2i &pos) {
  const auto pred = [pos](const auto &trace) { return trace.is_near_point(pos); };

  traces.erase(std::remove_if(traces.begin(), traces.end(), pred), traces.end());
}
void RecordingWindow::auto_shrink_traces() {
  for (auto &[trace, pos, color] : traces) {
    // We need to make sure, the block is inside the frame
    const auto test_fct = [Nx = Nx(), Ny = Ny(), w = Trace::width()](const Vec2i &v) {
      auto lim = [](auto x, auto N) { return ((x > 0) && (x < N)); };
      return (lim(v[0], Nx) && lim(v[1], Ny));
    };

    auto w      = Trace::width();
    Vec2i start = {pos[0] - w / 2, pos[1] - w / 2};
    // shrink the trace block width if it is too large
    while (!test_fct(start) || !test_fct(start + Vec2i(w, w))) {
      w -= 1;
      Trace::width(w);
      start = {pos[0] - w / 2, pos[1] - w / 2};
    }
  }
}
