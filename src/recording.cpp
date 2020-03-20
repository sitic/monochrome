#include "recording.h"

#include "filereader_formats.h"

void RotationCtrl::set_rotation(short rotation) {
  const std::array<short, 4> valid_rotations = {{0, 90, 180, 270}};
  if (std::none_of(valid_rotations.begin(), valid_rotations.end(),
                   [rotation](auto r) { return r == rotation; })) {
    throw std::logic_error(
        "set_rotation() has to be called with either "
        "0, 90, 180,  or 270 as parameter");
  }

  _rotation = rotation;
}

void RotationCtrl::add_rotation(short delta_rotation) {
  if (std::abs(delta_rotation) != 90) {
    throw std::logic_error(
        "add_rotation() has to be called with either "
        "+90 or -90 as parameter");
  }

  if (_rotation == 0 && delta_rotation < 0) {
    _rotation = 270;
  } else if (_rotation == 270 && delta_rotation > 0) {
    _rotation = 0;
  } else {
    _rotation += delta_rotation;
  }
}

void RotationCtrl::fliplr() {
  if (_rotation == 0 || _rotation == 180) {
    _fliplr = !_fliplr;
  } else {
    _flipud = !_flipud;
  }
}

void RotationCtrl::flipud() {
  if (_rotation == 0 || _rotation == 180) {
    _flipud = !_flipud;
  } else {
    _fliplr = !_fliplr;
  }
}

void RotationCtrl::apply(Eigen::MatrixXf& arr) {
  if (_fliplr && _flipud) {
    arr = arr.rowwise().reverse().colwise().reverse().eval();
  } else if (_fliplr) {
    arr.rowwise().reverseInPlace();
  } else if (_flipud) {
    arr.colwise().reverseInPlace();
  }

  switch (_rotation) {
    case 0:
      // do nothing
      break;
    case 90:
      arr = arr.transpose().colwise().reverse().eval();
      break;
    case 180:
      arr = arr.transpose().colwise().reverse().transpose().colwise().reverse().eval();
      break;
    case 270:
      arr = arr.transpose().rowwise().reverse().eval();
      break;
    default:
      throw std::logic_error("Invalid rotation value! (Should be 0, 90, 180, or 270");
  }
}

std::shared_ptr<AbstractRecording> Recording::autoguess_filerecording(const fs::path &path) {
  std::shared_ptr<AbstractRecording> file = std::make_shared<RawFileRecording>(path);
  if (!file->good()) {
    file = std::make_shared<BmpFileRecording>(path);
  }

  if (!file->error_msg().empty()) {
    new_ui_message(file->error_msg());
  }

  return file;
}

void Recording::load_frame(long t) {
  frame   = file->read_frame(t);
  t_frame = t;
  apply_rotation();
}

void Recording::load_next_frame(float speed) {
  _tf += speed;

  while (_tf > length() - 1) {
    _tf -= length() - 1;
    _t = 0;
  }

  if (std::floor(_tf) > _t) {
    _t = std::floor(_tf);
  }

  if (_t < 0) {
    // should never happen, but just in case
    _t  = 0;
    _tf = 0;
  }

  load_frame(_t);
}

void Recording::restart() {
  _tf = std::numeric_limits<float>::lowest();
  _t  = std::numeric_limits<int>::lowest();
}

bool Recording::export_ROI(fs::path path, Vec2i start, Vec2i size, Vec2i t0tmax, Vec2f minmax) {
  if (start[0] < 0 || start[1] < 0 || start[0] + size[0] > Nx() || start[1] + size[1] > Ny()) {
    new_ui_message(
        "ERROR: export_ROI() called with invalid array sizes, "
        "start={}, size={}",
        start, size);
    return false;
  }

  if (t0tmax[0] < 0 || t0tmax[1] > length() || t0tmax[0] > t0tmax[1]) {
    new_ui_message("ERROR: start or end frame invalid, start frame {}, end frame {}", t0tmax[0],
                   t0tmax[1]);
    return false;
  }

  fs::remove(path);
  std::ofstream out(path.string(), std::ios::out | std::ios::binary);
  auto cur_frame = t_frame;

  for (int t = t0tmax[0]; t < t0tmax[1]; t++) {
    load_frame(t);
    auto block = frame.block(start[0], start[1], size[0], size[1]);

    if (minmax[0] != minmax[1]) {
      auto normalize = [min = minmax[0], max = minmax[1]](const float &val) {
        return (val - min) / (max - min);
      };
      block = block.unaryExpr(normalize);
    }

    out.write(reinterpret_cast<const char *>(block.data()), block.size() * sizeof(float));
  }
  load_frame(cur_frame);

  if (!out.good()) {
    new_ui_message("ERROR: the writing to file {} seems to have failed", path.string());
    return false;
  }

  return true;
}
