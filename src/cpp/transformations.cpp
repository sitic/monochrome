#include "transformations.h"

std::shared_ptr<Transformation::Base> Transformation::factory(Transformations type, Recording &rec) {
  switch (type) {
    case Transformations::None:
      return std::make_shared<None>(rec);
    case Transformations::Gauss:
      return std::make_shared<GaussFilter>(rec);
    case Transformations::Mean:
      return std::make_shared<MeanFilter>(rec);
    case Transformations::Median:
      return std::make_shared<MedianFilter>(rec);
    case Transformations::FrameDiff:
      return std::make_shared<FrameDiff>(rec);
    case Transformations::ContrastEnhancement:
      return std::make_shared<ContrastEnhancement>(rec);
    default:
      throw std::logic_error("invalid transformation type");
  }
}

void Transformation::ContrastEnhancement::compute(const Eigen::MatrixXf &new_frame,
                                                  long new_frame_counter) {
  // Used fixed size versions for common variants
  if (kernel_size == 3) {
    functions::contrast_enhancement<3>(new_frame, frame, maskVersion);
  } else if (kernel_size == 5) {
    functions::contrast_enhancement<5>(new_frame, frame, maskVersion);
  } else if (kernel_size == 7) {
    functions::contrast_enhancement<7>(new_frame, frame, maskVersion);
  } else if (kernel_size == 9) {
    functions::contrast_enhancement<9>(new_frame, frame, maskVersion);
  } else {
    functions::contrast_enhancement(new_frame, frame, kernel_size, maskVersion);
  }
}

namespace Transformation {
  namespace functions {
    template <typename Derived, typename Derived2>
    auto minmax_masked(const Eigen::MatrixBase<Derived> &block,
                       const Eigen::MatrixBase<Derived2> &mask) {
      assert(mask.rows() == block.rows());
      assert(mask.cols() == block.cols());

      using Scalar = typename Derived::Scalar;
      Scalar max   = std::numeric_limits<Scalar>::lowest();
      Scalar min   = std::numeric_limits<Scalar>::max();

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

    Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> genCircleMask(const int kernel_size) {
      using MaskMatrix = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>;

      const bool is_odd = kernel_size % 2;
      if (!is_odd) {
        return MaskMatrix::Ones(kernel_size, kernel_size);
      }

      const Vec2i center = {kernel_size / 2, kernel_size / 2};
      const int radius2  = (kernel_size / 2) * (kernel_size / 2);

      MaskMatrix circleblock = MaskMatrix::Zero(kernel_size, kernel_size);
      for (long row = 0; row < kernel_size; row++) {
        for (long col = 0; col < kernel_size; col++) {
          const auto v          = Vec2i(row, col) - center;
          circleblock(row, col) = v.length_squared() <= radius2;
        }
      }

      return circleblock;
    }

    Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> genCircleMask2(const int kernel_size) {
      using MaskMatrix = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>;

      const bool is_odd = kernel_size % 2;
      if (!is_odd) {
        return MaskMatrix::Ones(kernel_size, kernel_size);
      }

      const Vec2i center = {kernel_size / 2, kernel_size / 2};
      const int radius2  = (kernel_size / 2 + 1) * (kernel_size / 2);

      MaskMatrix circleblock = MaskMatrix::Zero(kernel_size, kernel_size);
      for (long row = 0; row < kernel_size; row++) {
        for (long col = 0; col < kernel_size; col++) {
          const auto v          = Vec2i(row, col) - center;
          circleblock(row, col) = v.length_squared() <= radius2;
        }
      }

      return circleblock;
    }

    template <std::size_t kernel_size>
    void contrast_enhancement(const Eigen::MatrixXf &in, Eigen::MatrixXf &out, int maskVersion) {
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
          const auto block      = in.block<kernel_size, kernel_size>(row - offset, col - offset);
          const auto [min, max] = minmax_masked(block, mask);
          out(row, col)         = (in(row, col) - min) / (max - min);
        }
      }

      //      for (long row = 0; row < kernel_size; row++) {
      //        for (long col = 0; col < kernel_size; col++) {
      //          out(row, col) = mask(row, col) ? 1e3 : -1e3;
      //        }
      //      }
    }

    void contrast_enhancement(const Eigen::MatrixXf &in,
                              Eigen::MatrixXf &out,
                              const unsigned kernel_size,
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

      const bool is_odd = kernel_size % 2;
      const long offset = is_odd ? (kernel_size - 1) / 2 : kernel_size / 2;
      for (long row = offset; row < in.rows() - offset; row++) {
        for (long col = offset; col < in.cols() - offset; col++) {
          const auto block      = in.block(row - offset, col - offset, kernel_size, kernel_size);
          const auto [min, max] = minmax_masked(block, mask);
          out(row, col)         = (in(row, col) - min) / (max - min);
        }
      }

      //      for (long row = 0; row < kernel_size; row++) {
      //        for (long col = 0; col < kernel_size; col++) {
      //          out(row, col) = mask(row, col) ? 1e3 : -1e3;
      //        }
      //      }
    }

    void gauss_conv(const Eigen::MatrixXf &in,
                    Eigen::MatrixXf &buffer,
                    Eigen::MatrixXf &out,
                    const deriche_coeffs &c) {
      deriche_gaussian_conv_image(c, out.data(), buffer.data(), in.data(), in.rows(), in.cols(), 1);
    }

    void mean_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out, const unsigned kernel_size) {
      const bool is_odd = kernel_size % 2;
      const long offset = is_odd ? (kernel_size - 1) / 2 : kernel_size / 2;
      for (long row = offset; row < in.rows() - offset; row++) {
        for (long col = offset; col < in.cols() - offset; col++) {
          const auto block = in.block(row - offset, col - offset, kernel_size, kernel_size);
          out(row, col)    = block.mean();
        }
      }
    }

    void median_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out, const unsigned kernel_size) {
      assert(kernel_size > 0);

      std::vector<float> copy(kernel_size * kernel_size);
      const bool is_odd = kernel_size % 2;
      const long offset = is_odd ? (kernel_size - 1) / 2 : kernel_size / 2;
      for (long row = offset; row < in.rows() - offset; row++) {
        for (long col = offset; col < in.cols() - offset; col++) {
          const auto block = in.block(row - offset, col - offset, kernel_size, kernel_size);
          auto v           = block.reshaped();
          std::copy(v.cbegin(), v.cend(), copy.begin());
          auto n = kernel_size * kernel_size / 2;
          std::nth_element(copy.begin(), copy.begin() + n, copy.end());

          out(row, col) = copy[n];
        }
      }
    }
  }  // namespace functions
}  // namespace Transformation