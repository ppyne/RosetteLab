#pragma once

#include "rosettelab/core/geometry.hpp"

namespace rosettelab::curves {

struct DropletRosetteParameters {
    int droplets{3};
    double outer_radius{80.0};
    double rotation_degrees{-90.0};

    friend constexpr bool operator==(
        const DropletRosetteParameters&, const DropletRosetteParameters&) = default;
};

// Generates one closed cubic-Bezier subpath per droplet using only arcs of the
// outer circle and a ring of equal mutually tangent inner circles.
[[nodiscard]] core::BezierPath generate_droplet_rosette_bezier(
    const DropletRosetteParameters& parameters);

} // namespace rosettelab::curves
