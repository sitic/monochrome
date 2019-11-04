#pragma once

#include "contrast_enhancement.h"
#include "recording.h"

enum class FrameTransformations : int { None, FrameDiff, ContrastEnhancement };
const char *FrameTransformationsNames[] = {"None", "FrameDiff",
                                           "ContrastEnhancement"};

namespace FrameTransformation {

class Base {
public:
  Eigen::MatrixXf frame;
  float min = 0;
  float max = 1;

  Base() = default;
  virtual void assign(Recording &rec) = 0;
  virtual void compute(const Eigen::MatrixXf &new_frame,
                       long new_frame_counter) = 0;
};

class None : public Base {
public:
  void assign(Recording &rec) final {
    rec.load_frame(rec.length() / 2);
    max = rec.frame.maxCoeff();
    min = rec.frame.minCoeff();
  }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
  }
};

class FrameDiff : public Base {
private:
  Eigen::MatrixXf prev_frame;
  long t_prev_frame = 0;

public:
  void assign(Recording &rec) final {
    rec.load_frame(rec.length() / 2);
    prev_frame = rec.frame;
    t_prev_frame = rec.length() / 2;

    rec.load_frame(rec.length() / 2 + 2);
    compute(rec.frame, rec.length() / 2 + 2);
    max = std::max(std::abs(frame.minCoeff()), std::abs(frame.maxCoeff()));
    min = -max;
  }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    if (t_prev_frame != new_frame_counter) {
      frame = new_frame - prev_frame;
      prev_frame = new_frame;
      t_prev_frame = new_frame_counter;
    }
  }
};

class ContrastEnhancement : public Base {
public:
  static unsigned kernel_size;

  void assign(Recording &rec) final { frame.setZero(rec.Nx(), rec.Ny()); }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    // Used fixed size versions for
    if (kernel_size == 3) {
      contrast_enhancement<3>(new_frame, frame);
    } else if (kernel_size == 5) {
      contrast_enhancement<5>(new_frame, frame);
    } else {
      contrast_enhancement(new_frame, frame, kernel_size);
    }
  }

  void reset() { frame.setZero(); }
};
};