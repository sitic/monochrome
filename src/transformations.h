#pragma once

#include "recording.h"

extern "C" {
#include "ipol/gaussian_conv_deriche.h"
}

enum class Transformations : int { None, FrameDiff, ContrastEnhancement, FlickerSegmentation };
enum class Filters { None, Gauss, Mean, Median };

namespace Transformation {

  namespace functions {
    template <std::size_t kernel_size>
    void contrast_enhancement(const Eigen::MatrixXf &in, Eigen::MatrixXf &out, int maskVersion);

    void contrast_enhancement(const Eigen::MatrixXf &in,
                              Eigen::MatrixXf &out,
                              const unsigned kernel_size,
                              int maskVersion);

    void gauss_conv(const Eigen::MatrixXf &in,
                    Eigen::MatrixXf &buffer,
                    Eigen::MatrixXf &out,
                    const deriche_coeffs &c);

    void mean_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out, const unsigned kernel_size);
    void median_filter(const Eigen::MatrixXf &in, Eigen::MatrixXf &out, const unsigned kernel_size);

    template <typename Derived, typename Derived2>
    Derived conv2d(const Eigen::MatrixBase<Derived> &I, const Eigen::MatrixBase<Derived2> &kernel);
  }  // namespace functions

  class Base {
   public:
    Eigen::MatrixXf frame;
    float min = 0;
    float max = 1;

    Base()                                                                         = default;
    virtual ~Base()                                                                = default;
    virtual void allocate(Recording &rec)                                          = 0;
    virtual void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) = 0;
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
      prev_frame   = rec.frame;
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
        frame        = new_frame - prev_frame;
        prev_frame   = new_frame;
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

    void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final;

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
      sigma           = s;
      const int K     = 3;
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
        prev_frame   = new_frame;
        t_prev_frame = new_frame_counter;
        return;
      }

      if (t_prev_frame == new_frame_counter) {
        return;
      } else if (new_frame_counter < t_prev_frame) {
        reset();
      }

      frame = new_frame;  // - prev_frame;

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

      prev_frame   = new_frame;
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
      m_n          = 0;
    }
  };
}  // namespace Transformation