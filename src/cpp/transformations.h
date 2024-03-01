#pragma once

#include <queue>

#include "recording.h"

extern "C" {
#include "ipol/gaussian_conv_deriche.h"
}

enum class Transformations { None, Gauss, Mean, Median, FrameDiff, ContrastEnhancement };

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
    struct QueueEntry {
      Eigen::MatrixXf frame;
      long t;

      QueueEntry(Eigen::MatrixXf frame_, long t_) : frame(std::move(frame_)), t(t_) {}
      QueueEntry(const QueueEntry &t) = delete;
      QueueEntry(QueueEntry &&t) : frame(std::move(t.frame)), t(t.t) {}
    };
    std::queue<QueueEntry> prev_frames;

   public:
    static inline int n_frame_diff = 2;
    // initial values from allocate() for min and max
    float hist_min = 0;
    float hist_max = 0;

    using Base::Base;
    FrameDiff(Recording &rec) : Base() { allocate(rec); }

    void allocate(Recording &rec) final {
      prev_frames = std::queue<QueueEntry>();

      rec.load_frame(rec.length() / 2);
      compute(rec.frame, rec.length() / 2);

      rec.load_frame(rec.length() / 2 + 2);
      compute(rec.frame, rec.length() / 2 + 2);
      max = std::max(std::abs(frame.minCoeff()), std::abs(frame.maxCoeff()));
      min = -max;

      hist_min = min * 1.5f;
      hist_max = max * 1.5f;
    }

    void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final {
      while (n_frame_diff < prev_frames.size()) {
        prev_frames.pop();
      }
      while (n_frame_diff > prev_frames.size()) {
        prev_frames.emplace(new_frame, new_frame_counter);
      }

      if (prev_frames.front().t != new_frame_counter) {
        // TODO: this is really ugly (is it undefined behavior?), look for some replacement
        auto front = std::move(prev_frames.front());
        prev_frames.pop();
        frame       = new_frame - front.frame;
        front.frame = new_frame;
        front.t     = new_frame_counter;
        prev_frames.push(std::move(front));
      }
    }
  };

  class ContrastEnhancement : public Base {
   public:
    using Base::Base;
    ContrastEnhancement(Recording &rec) : Base() { allocate(rec); }

    static inline unsigned kernel_size = 3;
    static inline int maskVersion      = 2;

    void allocate(Recording &rec) final { frame.setZero(rec.Nx(), rec.Ny()); }

    void compute(const Eigen::MatrixXf &new_frame, long new_frame_counter) final;

    void reset() { frame.setZero(); }
  };

  class GaussFilter : public Base {
   private:
    static inline deriche_coeffs c;
    static inline float sigma = 1;
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
    static inline unsigned kernel_size = 3;

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
    static inline unsigned kernel_size = 3;

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

  std::shared_ptr<Base> factory(Transformations type, Recording &rec);
}  // namespace Transformation