#include <fstream>

#include "recording.h"

void RotationCtrl::set_rotation(short rotation) {
  const std::array<short, 4> valid_rotations = {{0, 90, 180, 270}};
  if (std::none_of(valid_rotations.begin(), valid_rotations.end(),
                   [rotation](auto r) { return r == rotation; })) {
    throw std::logic_error(
        "set_rotation() has to be called with either "
        "0, 90, 180, or 270 as parameter");
  }

  _rotation = rotation;
}

void RotationCtrl::add_rotation(short delta_rotation) {
  if (delta_rotation == 0) return;
  if (std::abs(delta_rotation) != 90) {
    throw std::logic_error(
        "add_rotation() has to be called with either "
        "0, +90 or -90 as parameter");
  }

  if (_fliplr != _flipud) {  // XOR, consistent behavior when flipped
    delta_rotation = -delta_rotation;
  }

  if (_rotation == 0 && delta_rotation < 0) {
    _rotation = 270;
  } else if (_rotation == 270 && delta_rotation > 0) {
    _rotation = 0;
  } else {
    _rotation += delta_rotation;
  }
}

void RotationCtrl::flip_lr() {
  _fliplr = !_fliplr;
}

void RotationCtrl::flip_ud() {
  _flipud = !_flipud;
}

void RotationCtrl::flip_reset() {
  _fliplr = false;
  _flipud = false;
}

void RotationCtrl::apply(Eigen::MatrixXf &arr) {
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

  if (_fliplr && _flipud) {
    arr = arr.rowwise().reverse().colwise().reverse().eval();
  } else if (_fliplr) {
    arr.rowwise().reverseInPlace();
  } else if (_flipud) {
    arr.colwise().reverseInPlace();
  }
}
std::pair<float, float> RotationCtrl::flow_signs() {
  float signx = +1;
  float signy = +1;
  switch (_rotation) {
    case 0:
      // do nothing
      break;
    case 90:
      signx = -1;
      break;
    case 180:
      signx = -1;
      signy = -1;
      break;
    case 270:
      signy = -1;
      break;
    default:
      throw std::logic_error("Invalid rotation value! (Should be 0, 90, 180, or 270");
  }

  if (_fliplr && _flipud) {
    signx *= -1;
    signy *= -1;
  } else if (_fliplr) {
    signy *= -1;
  } else if (_flipud) {
    signx *= -1;
  }

  return {signx, signy};
}

void Recording::load_frame(long t) {
  frame   = _file->read_frame(t);
  t_frame = t;
  apply_rotation();
}

bool Recording::export_ROI(
    fs::path path, Vec2i start, Vec2i size, Vec2i t0tmax, ExportFileType exportType, Vec2f minmax) {
  if (start[0] < 0 || start[1] < 0 || start[0] + size[0] > Nx() || start[1] + size[1] > Ny()) {
    global::new_ui_message("ERROR: export_ROI() called with invalid array sizes, start={}, size={}",
                           start, size);
    return false;
  }

  if (t0tmax[0] < 0 || t0tmax[1] > length() || t0tmax[0] > t0tmax[1]) {
    global::new_ui_message("ERROR: start or end frame invalid, start frame {}, end frame {}",
                           t0tmax[0], t0tmax[1]);
    return false;
  }

  fs::remove(path);
  std::ofstream out(path.string(), std::ios::out | std::ios::binary);
  // write file header if necessary
  if (exportType == ExportFileType::Npy) {
    unsigned long nt = t0tmax[1] - t0tmax[0];  // t0tmax[1] > t0tmax[0] guaranteed
    unsigned long ny = size[1];
    unsigned long nx = size[0];
    auto header      = create_npy_fileheader_float({nt, ny, nx});
    out << header;
  }

  auto cur_frame = t_frame;
  for (int t = t0tmax[0]; t < t0tmax[1]; t++) {
    load_frame(t);
    Eigen::MatrixXf block = frame.block(start[0], start[1], size[0], size[1]);
    if (minmax[0] != minmax[1]) {
      auto normalize = [min = minmax[0], max = minmax[1]](const float &val) {
        auto result = (val - min) / (max - min);
        return std::max(std::min(result, 1.f), 0.f);
      };
      block = block.unaryExpr(normalize);
    }
    out.write(reinterpret_cast<const char *>(block.data()), block.size() * sizeof(float));
  }
  load_frame(cur_frame);

  if (!out.good()) {
    global::new_ui_message("ERROR: the writing to file {} seems to have failed", path.string());
    return false;
  }

  return true;
}
