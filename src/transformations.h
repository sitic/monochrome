#pragma once

#include "recording.h"

extern "C" {
#include "ipol/gaussian_conv_deriche.h"
}

enum class Transformations : int { None, FrameDiff, ContrastEnhancement };
enum class Filters { None, Gauss, Mean, Median };

namespace Transformation {

namespace functions {
template <std::size_t kernel_size>
void contrast_enhancement(const Eigen::MatrixXf &in, Eigen::MatrixXf &out);

void contrast_enhancement(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                          const unsigned kernel_size);

void gauss_conv(const Eigen::MatrixXf &in, Eigen::MatrixXf &buffer,
                Eigen::MatrixXf &out, const deriche_coeffs &c) {
  deriche_gaussian_conv_image(c, out.data(), buffer.data(), in.data(),
                              in.rows(), in.cols(), 1);
}

template <std::size_t kernel_size>
void mean_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out);

void mean_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                 const unsigned kernel_size);
void median_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                   const unsigned kernel_size);

template <typename Derived, typename Derived2>
Derived conv2d(const Eigen::MatrixBase<Derived> &I,
               const Eigen::MatrixBase<Derived2> &kernel);
} // namespace functions

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

  // initial values from assign() for min and max
  float m_min_init = 0;
  float m_max_init = 0;

public:
  void assign(Recording &rec) final {
    rec.load_frame(rec.length() / 2);
    prev_frame = rec.frame;
    t_prev_frame = rec.length() / 2;

    rec.load_frame(rec.length() / 2 + 2);
    compute(rec.frame, rec.length() / 2 + 2);
    max = std::max(std::abs(frame.minCoeff()), std::abs(frame.maxCoeff()));
    min = -max;

    m_min_init = min;
    m_max_init = max;
  }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    if (t_prev_frame != new_frame_counter) {
      frame = new_frame - prev_frame;
      prev_frame = new_frame;
      t_prev_frame = new_frame_counter;
    }
  }

  float min_init() { return m_min_init; };
  float max_init() { return m_max_init; };
};

class ContrastEnhancement : public Base {
public:
  static unsigned kernel_size;

  void assign(Recording &rec) final { frame.setZero(rec.Nx(), rec.Ny()); }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    // Used fixed size versions for
    if (kernel_size == 3) {
      functions::contrast_enhancement<3>(new_frame, frame);
    } else if (kernel_size == 5) {
      functions::contrast_enhancement<5>(new_frame, frame);
    } else {
      functions::contrast_enhancement(new_frame, frame, kernel_size);
    }
  }

  void reset() { frame.setZero(); }
};

class GaussFilter : public Base {
private:
  static deriche_coeffs c;
  static float sigma;
  Eigen::MatrixXf buffer;

public:
  void assign(Recording &rec) final {
    set_sigma(sigma);
    frame.setZero(rec.Nx(), rec.Ny());
    buffer.setZero(rec.Nx(), rec.Ny());
  }

  static float get_sigma() { return sigma; }
  static void set_sigma(float s) {
    sigma = s;
    const int K = 3;
    const float tol = 1e-3;
    deriche_precomp(&c, s, K, tol);
  }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    functions::gauss_conv(new_frame, buffer, frame, c);
  }
};

class MeanFilter : public Base {
public:
  static unsigned kernel_size;

  void assign(Recording &rec) final { frame.setZero(rec.Nx(), rec.Ny()); }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    // Used fixed size versions for
    if (kernel_size == 3) {
      functions::mean_filter<3>(new_frame, frame);
    } else if (kernel_size == 5) {
      functions::mean_filter<5>(new_frame, frame);
    } else {
      functions::mean_filter(new_frame, frame, kernel_size);
    }
  }

  void reset() { frame.setZero(); }
};

class MedianFilter : public Base {
public:
  static unsigned kernel_size;

  void assign(Recording &rec) final { frame.setZero(rec.Nx(), rec.Ny()); }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    if (kernel_size > 0) {
      functions::median_filter(new_frame, frame, kernel_size);
    }
  }

  void reset() { frame.setZero(); }
};

namespace functions {
template <std::size_t kernel_size>
void contrast_enhancement(const Eigen::MatrixXf &in, Eigen::MatrixXf &out) {
  for (long row = kernel_size; row < in.rows() - kernel_size; row++) {
    for (long col = kernel_size; col < in.cols() - kernel_size; col++) {
      const auto block = in.block<kernel_size, kernel_size>(row, col);
      const auto max = block.maxCoeff();
      const auto min = block.minCoeff();

      out(row, col) = (in(row, col) - min) / (max - min);
    }
  }
}

void contrast_enhancement(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                          const unsigned kernel_size) {
  for (long row = kernel_size; row < in.rows() - kernel_size; row++) {
    for (long col = kernel_size; col < in.cols() - kernel_size; col++) {
      const auto block = in.block(row, col, kernel_size, kernel_size);
      const auto max = block.maxCoeff();
      const auto min = block.minCoeff();

      out(row, col) = (in(row, col) - min) / (max - min);
    }
  }
}

template <typename Derived, typename Derived2>
Derived conv2d(const Eigen::MatrixBase<Derived> &I,
               const Eigen::MatrixBase<Derived2> &kernel) {
  Derived O = Derived::Zero(I.rows(), I.cols());

  typedef typename Derived::Scalar Scalar;
  typedef typename Derived2::Scalar Scalar2;

  int col = 0, row = 0;
  int KSizeX = kernel.rows();
  int KSizeY = kernel.cols();

  int limitRow = I.rows() - KSizeX;
  int limitCol = I.cols() - KSizeY;

  Derived2 block;
  Scalar normalization = kernel.sum();
  if (normalization < 1E-6) {
    normalization = 1;
  }
  for (row = KSizeX; row < limitRow; row++) {

    for (col = KSizeY; col < limitCol; col++) {
      Scalar b = (static_cast<Derived2>(I.block(row, col, KSizeX, KSizeY))
                      .cwiseProduct(kernel))
                     .sum();
      O.coeffRef(row, col) = b;
    }
  }

  return O / normalization;
}

template <std::size_t kernel_size>
void mean_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out) {
  for (long row = kernel_size; row < in.rows() - kernel_size; row++) {
    for (long col = kernel_size; col < in.cols() - kernel_size; col++) {
      const auto block = in.block<kernel_size, kernel_size>(row, col);
      out(row, col) = block.mean();
    }
  }
}

void mean_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                 const unsigned kernel_size) {
  for (long row = kernel_size; row < in.rows() - kernel_size; row++) {
    for (long col = kernel_size; col < in.cols() - kernel_size; col++) {
      const auto block = in.block(row, col, kernel_size, kernel_size);
      out(row, col) = block.mean();
    }
  }
}

void median_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                   const unsigned kernel_size) {
  assert(kernel_size > 0);

  std::vector<float> copy(kernel_size * kernel_size);
  for (long row = kernel_size; row < in.rows() - kernel_size; row++) {
    for (long col = kernel_size; col < in.cols() - kernel_size; col++) {
      const auto block = in.block(row, col, kernel_size, kernel_size);
      auto v = block.reshaped();
      std::copy(v.cbegin(), v.cend(), copy.begin());
      auto n = kernel_size * kernel_size / 2;
      std::nth_element(copy.begin(), copy.begin() + n, copy.end());

      out(row, col) = copy[n];
    }
  }
}
} // namespace functions
}