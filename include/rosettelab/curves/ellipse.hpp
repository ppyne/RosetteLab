#pragma once

#include "rosettelab/core/geometry.hpp"

namespace rosettelab::curves {

struct EllipseParameters {
    double radius_x{80.0};
    double radius_y{50.0};
    double rotation_degrees{0.0};
    double bezier_tolerance{0.05};
};

[[nodiscard]] core::BezierPath generate_ellipse_bezier(
    const EllipseParameters& parameters,
    double tolerance = 0.05);

} // namespace rosettelab::curves
