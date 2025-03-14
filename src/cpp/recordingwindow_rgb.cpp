#include "prm.h"
#include "utils/files.h"
#include "recordingwindow_rgb.h"

void RGBRecordingWindow::set_context(GLFWwindow *new_context) {
  auto *prev_window = glfwGetCurrentContext();
  RecordingWindow::set_context(new_context);
  glfwMakeContextCurrent(glcontext);
  frame_shader.remove();
  frame_shader = Shader::create(utils::get_rc_text_file("src/shaders/frame.vert.glsl"),
                                utils::get_rc_text_file("src/shaders/frame_rgb.frag.glsl"));
  frame_shader.use();
  frame_shader.setInt("texture0", 0);
  frame_shader.setFloat("norm", 1);
  checkGlError("init");
  glfwMakeContextCurrent(prev_window);
}

void RGBRecordingWindow::display() {
  if (!glcontext) throw std::runtime_error("No window set, but RecordingWindow::display() called");

  load_next_frame();
  long t                    = t_frame;
  Eigen::MatrixXf r_channel = frame;
  load_frame(t, 1);
  Eigen::MatrixXf g_channel = frame;
  load_frame(t, 2);
  Eigen::MatrixXf b_channel = frame;

  histogram.compute(frame.reshaped());

  /* render code */
  glfwMakeContextCurrent(window);
  glClear(GL_COLOR_BUFFER_BIT);
  glfwMakeContextCurrent(glcontext);

  frame_shader.use();
  frame_shader.setFloat("norm", (bitrange == BitRange::U8)? 255: 1);
  // Option 1: Continue using separate textures
  // glActiveTexture(GL_TEXTURE0);
  // glBindTexture(GL_TEXTURE_2D, texture);
  // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Nx(), Ny(), GL_RED, GL_FLOAT, r_channel.data());
  // glActiveTexture(GL_TEXTURE1);
  // glBindTexture(GL_TEXTURE_2D, ctexture);
  // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Nx(), Ny(), GL_RED, GL_FLOAT, g_channel.data());
  // glActiveTexture(GL_TEXTURE2);
  // glBindTexture(GL_TEXTURE_2D, ctexturediff);
  // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Nx(), Ny(), GL_RED, GL_FLOAT, b_channel.data());

  // Option 2: Create a combined RGB texture
  const int numPixels = Nx() * Ny();
  std::vector<float> rgbData(numPixels * 3);

  // Interleave the RGB data
  for (int i = 0; i < numPixels; i++) {
    rgbData[i * 3]     = r_channel.data()[i];  // R
    rgbData[i * 3 + 1] = g_channel.data()[i];  // G
    rgbData[i * 3 + 2] = b_channel.data()[i];  // B
  }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Nx(), Ny(), GL_RGB, GL_FLOAT, rgbData.data());

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

  if (!traces.empty()) {
    throw std::runtime_error("Traces not supported in RGBRecordingWindow");
  }

  // Optical flow
  for (const auto &flow : flows) {
    if (!flow.show) continue;
    points_vert.clear();

    flow.data->load_frame(t_frame, 0);
    Eigen::MatrixXf u = flow.data->frame;  // force a copy
    flow.data->load_frame(t_frame, 1);
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

  // Points video
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

void RGBRecordingWindow::update_gl_texture() {
  if (!glcontext) return;
  auto *prev_window = glfwGetCurrentContext();
  glfwMakeContextCurrent(glcontext);
  if (texture) {
    glDeleteTextures(1, &texture);
  }

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, Nx(), Ny(), 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // GL_LINEAR
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // GL_LINEAR
  glfwMakeContextCurrent(prev_window);
}