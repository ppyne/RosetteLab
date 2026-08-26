#pragma once

#include "rosettelab/core/geometry.hpp"

namespace rosettelab::curves {

struct HarmonographParameters {
    double amplitude_x{80.0};
    double amplitude_y{80.0};
    double frequency_x{3.0};
    double frequency_y{2.0};
    double phase_x_degrees{90.0};
    double phase_y_degrees{0.0};
    double damping_x{0.015};
    double damping_y{0.010};
    double duration{40.0};
    double rotation_degrees{0.0};
    double bezier_tolerance{0.05};

    friend constexpr bool operator==(const HarmonographParameters&, const HarmonographParameters&) = default;
};

[[nodiscard]] core::BezierPath generate_harmonograph_bezier(
    const HarmonographParameters& parameters,
    double tolerance = 0.05);

} // namespace rosettelab::curves
