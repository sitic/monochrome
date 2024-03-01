#include <random>

#include "recordingwindow_helpers.h"
#include "prm.h"

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
  synchronize_with(other, true);
  return *this;
}
void RecordingPlaybackCtrl::synchronize_with(const RecordingPlaybackCtrl &other, bool warn) {
  if (other.length_ != length_) {
    if (warn && length_ > 1)
      global::new_ui_message(
          "Synchronizing videos of unequal length, this might not work as expected");
  } else {
    t_  = other.t_;
    tf_ = other.tf_;
  }
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
  if (t >= length_ || t < 0) {
    t = 0;
  }

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

std::pair<float, float> oportunistic_minmax(std::shared_ptr<AbstractFile> file,
                                            int sampling_frames) {
  auto Nt         = file->length();
  sampling_frames = std::min(sampling_frames, Nt);

  std::vector<long> all_indices(Nt);
  for (long i = 0; i < Nt; i++) {
    all_indices[i] = i;
  }

  std::vector<long> indices;
  std::sample(all_indices.begin(), all_indices.end(), std::back_inserter(indices), sampling_frames,
              std::mt19937{std::random_device{}()});

  float min = std::numeric_limits<float>::max();
  float max = std::numeric_limits<float>::lowest();

  for (auto i : indices) {
    auto frame = file->read_frame(i);
    auto [frame_min, frame_max] =
        utils::minmax_element_skipNaN(frame.data(), frame.data() + frame.size());
    if (frame_min < min) {
      min = frame_min;
    }
    if (frame_max > max) {
      max = frame_max;
    }
  }

  return {min, max};
}