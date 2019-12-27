#pragma once

#include "recording.h"

extern "C" {
#include "ipol/gaussian_conv_deriche.h"
}

enum class Transformations : int {
  None,
  FrameDiff,
  ContrastEnhancement,
  FlickerSegmentation
};
enum class Filters { None, Gauss, Mean, Median };

namespace Transformation {

namespace functions {
template <std::size_t kernel_size>
void contrast_enhancement(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                          int maskVersion);

void contrast_enhancement(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                          const unsigned kernel_size, int maskVersion);

void gauss_conv(const Eigen::MatrixXf &in, Eigen::MatrixXf &buffer,
                Eigen::MatrixXf &out, const deriche_coeffs &c) {
  deriche_gaussian_conv_image(c, out.data(), buffer.data(), in.data(),
                              in.rows(), in.cols(), 1);
}

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
  virtual void allocate(Recording &rec) = 0;
  virtual void compute(const Eigen::MatrixXf &new_frame,
                       long new_frame_counter) = 0;
};

class None : public Base {
public:
  using Base::Base;
  None(Recording &rec) : Base() { allocate(rec); }

  void allocate(Recording &rec) final {
    rec.load_frame(rec.length() / 2);
    max = rec.frame.maxCoeff();
    min = rec.frame.minCoeff();
  }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    frame = new_frame;
  }
};

class FrameDiff : public Base {
private:
  Eigen::MatrixXf prev_frame;
  long t_prev_frame = 0;

  // initial values from allocate() for min and max
  float m_min_init = 0;
  float m_max_init = 0;

public:
  using Base::Base;
  FrameDiff(Recording &rec) : Base() { allocate(rec); }

  void allocate(Recording &rec) final {
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
  using Base::Base;
  ContrastEnhancement(Recording &rec) : Base() { allocate(rec); }

  static unsigned kernel_size;
  static int maskVersion;

  void allocate(Recording &rec) final { frame.setZero(rec.Nx(), rec.Ny()); }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    // Used fixed size versions for common variants
    if (kernel_size == 3) {
      functions::contrast_enhancement<3>(new_frame, frame, maskVersion);
    } else if (kernel_size == 5) {
      functions::contrast_enhancement<5>(new_frame, frame, maskVersion);
    } else {
      functions::contrast_enhancement(new_frame, frame, kernel_size,
                                      maskVersion);
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
  using Base::Base;
  GaussFilter(Recording &rec) : Base() { allocate(rec); }

  void allocate(Recording &rec) final {
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

  using Base::Base;
  MeanFilter(Recording &rec) : Base() { allocate(rec); }

  void allocate(Recording &rec) final { frame.setZero(rec.Nx(), rec.Ny()); }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    functions::mean_filter(new_frame, frame, kernel_size);
  }

  void reset() { frame.setZero(); }
};

class MedianFilter : public Base {
public:
  static unsigned kernel_size;

  using Base::Base;
  MedianFilter(Recording &rec) : Base() { allocate(rec); }

  void allocate(Recording &rec) final { frame.setZero(rec.Nx(), rec.Ny()); }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    if (kernel_size > 0) {
      functions::median_filter(new_frame, frame, kernel_size);
    }
  }

  void reset() { frame.setZero(); }
};

class FlickerSegmentation : public Base {
  int m_n = 0;

  Eigen::MatrixXf m_oldM, m_newM, m_oldS, m_newS;
  Eigen::MatrixXf mean;
  Eigen::MatrixXf prev_frame;
  long t_prev_frame = 0;

public:
  using Base::Base;
  FlickerSegmentation(Recording &rec) : Base() { allocate(rec); }

  void allocate(Recording &rec) final {
    frame.setZero(rec.Nx(), rec.Ny());
    m_n = 0;
  }

  void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
    m_n++;

    if (m_n == 1) {
      prev_frame = new_frame;
      t_prev_frame = new_frame_counter;
      return;
    }

    if (t_prev_frame == new_frame_counter) {
      return;
    } else if (new_frame_counter < t_prev_frame) {
      reset();
    }

    frame = new_frame; // - prev_frame;

    // See Knuth TAOCP vol 2, 3rd edition, page 232
    if (m_n == 2) {
      m_oldM = frame;
      m_newM = frame;
      m_oldS.setZero(frame.cols(), frame.rows());
    } else {
      m_newM = m_oldM + (frame - m_oldM) / m_n;
      m_newS = m_oldS + (frame - m_oldM).cwiseProduct(frame - m_newM);

      // set up for next iteration
      m_oldM = m_newM;
      m_oldS = m_newS;

      frame = m_newS.cwiseQuotient(m_newM) / (m_n - 1);
      // frame = m_newS/(m_n - 1);
    }

    prev_frame = new_frame;
    t_prev_frame = new_frame_counter;

    // if (!prev_frame.rows()) {
    //  prev_frame = new_frame;
    //  t_prev_frame = new_frame_counter;
    //  return;
    //}
    //
    // if (t_prev_frame != new_frame_counter) {
    //  frame += (new_frame - prev_frame).cwiseAbs();
    //  prev_frame = new_frame;
    //  t_prev_frame = new_frame_counter;
    //}
  }

  void reset() {
    frame.setZero();
    t_prev_frame = 0;
    m_n = 0;
  }
};

namespace functions {
template <typename Derived, typename Derived2>
auto minmax_masked(const Eigen::MatrixBase<Derived> &block,
                   const Eigen::MatrixBase<Derived2> &mask) {
  assert(mask.rows() == block.rows());
  assert(mask.cols() == block.cols());

  using Scalar = typename Derived::Scalar;
  Scalar max = std::numeric_limits<Scalar>::lowest();
  Scalar min = std::numeric_limits<Scalar>::max();

  for (int row = 0; row < block.rows(); row++) {
    for (int col = 0; col < block.cols(); col++) {
      if (mask(row, col)) {
        max = std::max(max, block(row, col));
        min = std::min(min, block(row, col));
      }
    }
  }
  return std::make_pair(min, max);
}

Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>
genCircleMask(const int kernel_size) {
  using MaskMatrix = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>;

  const bool is_odd = kernel_size % 2;
  if (!is_odd) {
    return MaskMatrix::Ones(kernel_size, kernel_size);
  }

  const Vec2i center = {kernel_size / 2, kernel_size / 2};
  const int radius2 = (kernel_size / 2) * (kernel_size / 2);

  MaskMatrix circleblock = MaskMatrix::Zero(kernel_size, kernel_size);
  for (long row = 0; row < kernel_size; row++) {
    for (long col = 0; col < kernel_size; col++) {
      const auto v = Vec2i(row, col) - center;
      circleblock(row, col) = v.length_squared() <= radius2;
    }
  }

  return circleblock;
}

Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>
genCircleMask2(const int kernel_size) {
  using MaskMatrix = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>;

  const bool is_odd = kernel_size % 2;
  if (!is_odd) {
    return MaskMatrix::Ones(kernel_size, kernel_size);
  }

  const Vec2i center = {kernel_size / 2, kernel_size / 2};
  const int radius2 = (kernel_size / 2 + 1) * (kernel_size / 2);

  MaskMatrix circleblock = MaskMatrix::Zero(kernel_size, kernel_size);
  for (long row = 0; row < kernel_size; row++) {
    for (long col = 0; col < kernel_size; col++) {
      const auto v = Vec2i(row, col) - center;
      circleblock(row, col) = v.length_squared() <= radius2;
    }
  }

  return circleblock;
}

template <std::size_t kernel_size>
void contrast_enhancement(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                          int maskVersion) {
  using MaskMatrix = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>;
  MaskMatrix mask;
  switch (maskVersion) {
  case 0:
    mask = MaskMatrix::Ones(kernel_size, kernel_size);
    break;
  case 1:
    mask = genCircleMask(kernel_size);
    break;
  case 2:
    mask = genCircleMask2(kernel_size);
    break;
  default:
    throw std::logic_error("invalid maskVersion");
  }

  const long offset = kernel_size % 2 ? (kernel_size - 1) / 2 : kernel_size / 2;
  for (long row = offset; row < in.rows() - offset; row++) {
    for (long col = offset; col < in.cols() - offset; col++) {
      const auto block =
          in.block<kernel_size, kernel_size>(row - offset, col - offset);
      const auto [min, max] = minmax_masked(block, mask);
      out(row, col) = (in(row, col) - min) / (max - min);
    }
  }

  for (long row = 0; row < kernel_size; row++) {
    for (long col = 0; col < kernel_size; col++) {
      out(row, col) = mask(row, col) ? 1e3 : -1e3;
    }
  }
}

void contrast_enhancement(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                          const unsigned kernel_size, int maskVersion) {
  using MaskMatrix = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>;
  MaskMatrix mask;
  switch (maskVersion) {
  case 0:
    mask = MaskMatrix::Ones(kernel_size, kernel_size);
    break;
  case 1:
    mask = genCircleMask(kernel_size);
    break;
  case 2:
    mask = genCircleMask2(kernel_size);
    break;
  default:
    throw std::logic_error("invalid maskVersion");
  }

  const bool is_odd = kernel_size % 2;
  const long offset = is_odd ? (kernel_size - 1) / 2 - 1 : kernel_size / 2;
  for (long row = offset; row < in.rows() - offset; row++) {
    for (long col = offset; col < in.cols() - offset; col++) {
      const auto block =
          in.block(row - offset, col - offset, kernel_size, kernel_size);
      const auto [min, max] = minmax_masked(block, mask);
      out(row, col) = (in(row, col) - min) / (max - min);
    }
  }

  for (long row = 0; row < kernel_size; row++) {
    for (long col = 0; col < kernel_size; col++) {
      out(row, col) = mask(row, col) ? 1e3 : -1e3;
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

void mean_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                 const unsigned kernel_size) {
  const bool is_odd = kernel_size % 2;
  const long offset = is_odd ? (kernel_size - 1) / 2 - 1 : kernel_size / 2;
  for (long row = offset; row < in.rows() - offset; row++) {
    for (long col = offset; col < in.cols() - offset; col++) {
      const auto block =
          in.block(row - offset, col - offset, kernel_size, kernel_size);
      out(row, col) = block.mean();
    }
  }
}

void median_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out,
                   const unsigned kernel_size) {
  assert(kernel_size > 0);

  std::vector<float> copy(kernel_size * kernel_size);
  const bool is_odd = kernel_size % 2;
  const long offset = is_odd ? (kernel_size - 1) / 2 - 1 : kernel_size / 2;
  for (long row = offset; row < in.rows() - offset; row++) {
    for (long col = offset; col < in.cols() - offset; col++) {
      const auto block =
          in.block(row - offset, col - offset, kernel_size, kernel_size);
      auto v = block.reshaped();
      std::copy(v.cbegin(), v.cend(), copy.begin());
      auto n = kernel_size * kernel_size / 2;
      std::nth_element(copy.begin(), copy.begin() + n, copy.end());

      out(row, col) = copy[n];
    }
  }
}
} // namespace functions
} // namespace Transformation