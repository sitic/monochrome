#pragma once

#include <Eigen/Dense>

using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using pixel = uint16;

using Vec2f = Eigen::Vector2f;
using Vec3f = Eigen::Vector3f;
using PixArray = Eigen::Matrix<pixel, Eigen::Dynamic, Eigen::Dynamic>;
using Pix2Array =
    Eigen::Matrix<Eigen::Vector2<pixel>, Eigen::Dynamic, Eigen::Dynamic>;