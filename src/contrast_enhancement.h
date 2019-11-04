#pragma once

#include "utils.h"

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
                          const unsigned kernel_size = 3) {
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