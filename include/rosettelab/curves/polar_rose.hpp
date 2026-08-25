#pragma once

#include "rosettelab/core/geometry.hpp"

#include <cstddef>

namespace rosettelab::curves {

struct PolarRoseParameters {
    double radius{100.0};
    double k{7.0};
    double phase_degrees{0.0};
    double rotation_degrees{0.0};
    std::size_t samples{720};
};

[[nodiscard]] core::Polyline generate_polar_rose(const PolarRoseParameters& parameters);

} // namespace rosettelab::curves

