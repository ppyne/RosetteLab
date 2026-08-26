#pragma once

#include "rosettelab/core/geometry.hpp"

namespace rosettelab::curves {

struct LissajousParameters {
    double amplitude_x{80.0};
    double amplitude_y{80.0};
    int frequency_x{3};
    int frequency_y{2};
    double phase_x_degrees{90.0};
    double phase_y_degrees{0.0};
    double rotation_degrees{0.0};
    double bezier_tolerance{0.05};
};

[[nodiscard]] core::BezierPath generate_lissajous_bezier(
    const LissajousParameters& parameters,
    double tolerance = 0.05);

} // namespace rosettelab::curves
