#include "recordingwindow.h"
#include "globals.h"

namespace prm {
  PlaybackCtrl playbackCtrl;
  extern double lastframetime;
}  // namespace prm

namespace global {
  extern GLFWwindow *main_window;
  extern std::vector<SharedRecordingPtr> recordings;
}  // namespace global

float RecordingWindow::scale_fct = 1;
int FlowData::skip               = 4;
float FlowData::pointsize        = 1.5;

namespace {
  SharedRecordingPtr rec_from_window_ptr(GLFWwindow *_window) {
    return *std::find_if(global::recordings.begin(), global::recordings.end(),
                         [_window](const auto &r) { return r->window == _window; });
  }

  Shader create_frame_shader() {
    // Shader sources
    std::string vertexSource   = R"glsl(
      #version 330 core
      layout (location = 0) in vec2 position;
      layout (location = 1) in vec2 texcoord;
      out vec2 Texcoord;

      void main() {
          Texcoord = texcoord;
          gl_Position = vec4(position, 0.0, 1.0);
      })glsl";
    std::string fragmentSource = R"glsl(
      #version 330 core
      out vec4 FragColor;
      in vec2 Texcoord;
      uniform sampler2D texture0;
      uniform sampler1D textureC;
      uniform vec2 minmax;

      void main() {
          float val = texture(texture0, Texcoord).r;
          if (isnan(val)) {
            //discard;
            FragColor = vec4(0.0, 0.0, 0.0, 0.0);
          } else {
            val = (val - minmax.x) / (minmax.y - minmax.x);
            FragColor = texture(textureC, clamp(val, 0.0, 1.0));
          }
      })glsl";
    return Shader::create(vertexSource, fragmentSource);
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
    std::string vertexSource   = R"glsl(
      #version 330 core
      layout (location = 0) in vec2 position;
      layout (location = 1) in vec3 aColor;

      out VS_OUT {
          vec3 color;
      } vs_out;

      void main()
      {
          vs_out.color = aColor;
          gl_Position = vec4(position, 0.0, 1.0);
      })glsl";
    std::string fragmentSource = R"glsl(
      #version 330 core
      out vec4 FragColor;

      in vec3 fColor;

      void main()
      {
          FragColor = vec4(fColor, 1.0);
      })glsl";
    std::string geometrySource = R"glsl(
      #version 330 core
      layout (points) in;
      layout (line_strip, max_vertices = 5) out;
      uniform vec2 halfwidth;

      in VS_OUT {
          vec3 color;
      } gs_in[];

      out vec3 fColor;

      void build_rect(vec4 position)
      {
          fColor = gs_in[0].color; // gs_in[0] since there's only one input vertex
          vec4 vert = position + vec4(-halfwidth.x, -halfwidth.y, 0.0, 0.0); // bottom-left
          gl_Position = clamp(vert, -1, 1);
          EmitVertex();
          vert = position + vec4(halfwidth.x, -halfwidth.y, 0.0, 0.0); // bottom-right
          gl_Position = clamp(vert, -1, 1);
          EmitVertex();
          vert = position + vec4(halfwidth.x, halfwidth.y, 0.0, 0.0); // top-right
          gl_Position = clamp(vert, -1, 1);
          EmitVertex();
          vert = position + vec4(-halfwidth.x, halfwidth.y, 0.0, 0.0); // top-left
          gl_Position = clamp(vert, -1, 1);
          EmitVertex();
          vert = position + vec4(-halfwidth.x, -halfwidth.y, 0.0, 0.0); // bottom-left
          gl_Position = clamp(vert, -1, 1);
          EmitVertex();
          EndPrimitive();
      }

      void main() {
          build_rect(gl_in[0].gl_Position);
      })glsl";
    return Shader::create(vertexSource, fragmentSource, geometrySource);
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

  Shader create_flow_shader() {
    std::string vertexSource = R"glsl(
      #version 330 core
      layout (location = 0) in vec2 position;

      void main()
      {
          gl_Position = vec4(position, 0.0, 1.0);
      })glsl";
    // https://rubendv.be/posts/fwidth/
    std::string fragmentSource = R"glsl(
      #version 330 core
      out vec4 FragColor;
      uniform vec4 color;

      void main()
      {
          vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
          float dist_squared = dot(circCoord, circCoord);
          float alpha = smoothstep(0.9, 1.1, dist_squared);
          FragColor = color;
          FragColor.a -= alpha;
      })glsl";
    return Shader::create(vertexSource, fragmentSource);
  }

  std::pair<GLuint, GLuint> create_flow_vaovbo() {
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

std::pair<int, float> RecordingPlaybackCtrl::next_timestep(float speed_) const {
  auto tf = tf_ + speed_;
  while (tf >= length_) {
    tf -= length_;
  }

  int t = std::floor(tf_);

  if (t < 0) {
    // should never happen, but just in case
    t  = 0;
    tf = 0;
  }
  return {t, tf};
}
RecordingPlaybackCtrl &RecordingPlaybackCtrl::operator=(const RecordingPlaybackCtrl &other) {
  if (other.t_ >= length_ - 1 || other.tf_ >= length_ - 1) {
    global::new_ui_message(
        "Synchronizing videos of unequal length, this might not work as expected");
    return *this;
  }
  t_  = other.t_;
  tf_ = other.tf_;
  return *this;
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
void RecordingPlaybackCtrl::set_next(int t) {
  t_  = std::numeric_limits<int>::lowest();
  tf_ = t - prm::playbackCtrl.val;
}
void RecordingPlaybackCtrl::restart() {
  t_  = std::numeric_limits<int>::lowest();
  tf_ = std::numeric_limits<float>::lowest();
}
bool RecordingPlaybackCtrl::is_last() const {
  return tf_ + prm::playbackCtrl.val >= length_;
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
  const std::array<Vec4f, 4> cycle_list = {{
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
  }

  return w;
}
std::pair<Vec2i, Vec2i> Trace::clamp(const Vec2i &pos, const Vec2i &max_size) {
  Vec2i size  = {Trace::width(), Trace::width()};
  Vec2i start = pos - size / 2;
  for (int i = 0; i < 2; i++) {
    if (start[i] < 0) {
      size[i] += start[i];
      start[i] = 0;
    }
    size[i] = std::max(std::min(size[i], max_size[i] - start[i]), 0);
  }
  return {start, size};
}

Vec4f FlowData::next_color(unsigned color_count) {
  // List of colors to cycle through
  const std::array<Vec4f, 5> cycle_list = {{
      {0, 0, 0, 1},
      {228 / 255.f, 26 / 255.f, 28 / 255.f, 1},
      {55 / 255.f, 126 / 255.f, 184 / 255.f, 1},
      {77 / 255.f, 175 / 255.f, 74 / 255.f, 1},
      {152 / 255.f, 78 / 255.f, 163 / 255.f, 1},
  }};

  if (color_count >= cycle_list.size()) {
    color_count %= cycle_list.size();
  }
  return cycle_list.at(color_count);
}

TransformationList::TransformationCtrl::TransformationCtrl(
    std::variant<Transformations, Filters> type, Recording &rec, int _gen)
    : m_gen(_gen), m_type(type) {
  if (auto *t = std::get_if<Transformations>(&type)) {
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
  } else if (auto *t = std::get_if<Filters>(&type)) {
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

RecordingWindow::RecordingWindow(std::shared_ptr<AbstractRecording> file_)
    : Recording(std::move(file_)), playback(good() ? length() : 0), transformationArena(*this) {
  if (!good()) {
    return;
  }

  if (Trace::width() == 0) {
    // if unset, set trace edge length to something reasonable
    auto val = std::min(Nx(), Ny()) / 64;
    Trace::width(std::max(val, 2));
  }

  if (file->bitrange()) {
    bitrange = file->bitrange().value();
  }
  float &min = get_min(Transformations::None);
  float &max = get_max(Transformations::None);
  if ((min == max) || std::isnan(min) || std::isnan(max)) {
    std::tie(min, max) = utils::bitrange_to_float(bitrange);
  }

  if (file->cmap()) {
    cmap_ = file->cmap().value();
  } else if (bitrange == BitRange::PHASE || bitrange == BitRange::PHASE_DIFF) {
    // assume that's its a phase map and the user prefers HSV in this circumstances
    cmap_ = ColorMap::HSV;
  }
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
  if (window != new_context) glfwHideWindow(window);
  if (glcontext && frame_shader) clear_gl_memory();

  glcontext = new_context;
  glfwMakeContextCurrent(glcontext);
  frame_shader                              = create_frame_shader();
  std::tie(frame_vao, frame_vbo, frame_ebo) = create_frame_vaovboebo();
  trace_shader                              = create_trace_shader();
  std::tie(trace_vao, trace_vbo)            = create_trace_vaovbo();
  flow_shader                               = create_flow_shader();
  std::tie(flow_vao, flow_vbo)              = create_flow_vaovbo();

  update_gl_texture();
  ColorMap cmap_tmp = cmap_;
  std::swap(ctexture, ctexturediff);
  colormap(ColorMap::DIFF);
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

void RecordingWindow::display(Filters prefilter,
                              Transformations transformation,
                              Filters postfilter) {
  if (!glcontext) throw std::runtime_error("No window set, but RecordingWindow::display() called");

  load_next_frame();

  Eigen::MatrixXf *arr = &frame;

  auto *pretransform = transformationArena.create_if_needed(prefilter, 0);
  pretransform->compute(*arr, t_frame);
  arr = &pretransform->frame;

  auto *transform = transformationArena.create_if_needed(transformation, 0);
  transform->compute(*arr, t_frame);
  arr = &transform->frame;

  switch (transformation) {
    case Transformations::None:
      std::tie(histogram.min, histogram.max) = utils::bitrange_to_float(bitrange);
      break;
    case Transformations::FrameDiff: {
      auto *frameDiff = dynamic_cast<Transformation::FrameDiff *>(transform);
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

  auto *posttransform = transformationArena.create_if_needed(postfilter, 1);
  posttransform->compute(*arr, t_frame);
  arr = &posttransform->frame;

  histogram.compute(arr->reshaped());

  // update traces
  if (playback.next_t() != playback.current_t()) {
    for (auto &[trace, pos, color] : traces) {
      auto [start, size] = Trace::clamp(pos, {Nx(), Ny()});
      if (size[0] > 0 && size[1] > 0) {
        auto block = arr->block(start[0], start[1], size[0], size[1]);
        trace.push_back(block.mean());
        //block.setConstant(0);  // for testing
      }
    }
  }

  /* render code */
  glfwMakeContextCurrent(window);
  glClear(GL_COLOR_BUFFER_BIT);
  glfwMakeContextCurrent(glcontext);

  frame_shader.use();
  frame_shader.setVec2("minmax", get_min(transformation), get_max(transformation));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Nx(), Ny(), GL_RED, GL_FLOAT, arr->data());
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D,
                transformation == Transformations::FrameDiff ? ctexturediff : ctexture);

  // Draw the frame
  glBindVertexArray(frame_vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

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
    for (auto &[trace, pos, color] : traces) {
      coordtrans(pos);
      trace_vert.push_back(color[0]);
      trace_vert.push_back(color[1]);
      trace_vert.push_back(color[2]);
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
    flow_vert.clear();

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

        flow_vert.push_back(xx);
        flow_vert.push_back(yy);
      }
    }

    flow_shader.use();
    flow_shader.setVec4("color", flow.color);
    glBindVertexArray(flow_vao);
    glBindBuffer(GL_ARRAY_BUFFER, flow_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * flow_vert.size(), flow_vert.data(),
                 GL_STREAM_DRAW);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glPointSize(FlowData::pointsize * scale_fct);
    glDrawArrays(GL_POINTS, 0, flow_vert.size() / 2);
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
    for (const auto &r : global::recordings) {
      r->resize_window();
    }
  }
}

void RecordingWindow::cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
  auto rec      = rec_from_window_ptr(window);
  rec->mousepos = {xpos, ypos};
  if (rec->mousebutton.left) {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    int x = rec->mousepos[0] * rec->Nx() / w;
    int y = rec->mousepos[1] * rec->Ny() / h;

    rec->add_trace_pos({x, y});
  }
}

void RecordingWindow::mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  auto rec = rec_from_window_ptr(window);
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
    auto rec = rec_from_window_ptr(window);
    auto fn  = rec->save_snapshot();
    global::new_ui_message("Saved screenshot to {}", fn.string());
  } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    prm::playbackCtrl.toggle_play_pause();
  }
}

void RecordingWindow::start_recording(const std::string &filename, int fps) {
  playback.set_next(export_ctrl.video.tstart);
  for (auto &child : children) {
    child->playback.set_next(export_ctrl.video.tstart);
  }
  export_ctrl.video.videoRecorder.start_recording(filename, window, fps);
  export_ctrl.video.recording = true;
  export_ctrl.video.progress  = 0;
  prm::lastframetime          = std::numeric_limits<float>::lowest();
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
  for (const auto &rec : global::recordings) {
    rec->rotation_was_changed();
  }
}
void RecordingWindow::add_rotation(short d_rotation) {
  rotations.add_rotation(d_rotation);
  for (const auto &rec : global::recordings) {
    rec->rotation_was_changed();
  }
}
void RecordingWindow::rotation_was_changed() {
  load_frame(t_frame);
  transformationArena.reallocate();
  resize_window();
  update_gl_texture();
  glfwSetWindowAspectRatio(window, Nx(), Ny());
  traces.clear();
}

void RecordingWindow::add_flow(std::shared_ptr<Recording> flow) {
  if (flow->length() >= 2 * length())
    flows.emplace_back(flow, flows.size());
  else
    global::new_ui_message("Failed to add '{}' as flow to '{}' because length does not match",
                           flow->name(), name());
}

void RecordingWindow::render() {
  if (glcontext == window) {
    glfwMakeContextCurrent(window);

    if (export_ctrl.video.recording) {
      if (playback.current_t() < 0) {
        // do nothing, this is the frame before the first one
      } else if (playback.is_last()) {
        stop_recording();
        global::new_ui_message("Exporting video finished!");
      } else {
        export_ctrl.video.videoRecorder.add_frame();
        export_ctrl.video.progress = playback.progress();
      }
    }

    glfwSwapBuffers(window);

    glfwMakeContextCurrent(global::main_window);
  }
}
