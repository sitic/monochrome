#pragma once

#include "utils/files.h"
#include "recordingwindow.h"

class RGBRecordingWindow : public RecordingWindow {
 public:
  RGBRecordingWindow(const fs::path &path) : RecordingWindow(path) {};
  RGBRecordingWindow(std::shared_ptr<AbstractFile> file_) : RecordingWindow(file_) {};
  virtual ~RGBRecordingWindow() = default;

  void add_trace(const Vec2i &pos) override {};
  void colormap(ColorMap cmap) override {};
  void set_transformation(Transformations type) override {};

  void set_context(GLFWwindow *new_context) override;

  void display() override;

 private:
  void update_gl_texture() override;
  void clear_gl_memory() override;

  std::array<GLuint, 3> textures_rgb = {GL_FALSE, GL_FALSE, GL_FALSE};
};

inline bool is_rgb_rec(SharedRecordingPtr rec) {
    return dynamic_cast<RGBRecordingWindow *>(rec.get()) != nullptr;
}