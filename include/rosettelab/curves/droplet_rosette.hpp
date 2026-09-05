#pragma once

#include "rosettelab/core/geometry.hpp"

namespace rosettelab::curves {

struct DropletRosetteParameters {
    int droplets{3};
    double outer_radius{80.0};
    double core_radius{16.0};
    double swirl_degrees{28.0};
    double width_percent{88.0};
    double roundness{0.55};
    double rotation_degrees{-90.0};

    friend constexpr bool operator==(
        const DropletRosetteParameters&, const DropletRosetteParameters&) = default;
};

// Generates one closed cubic-Bezier subpath per droplet. The outer edge is a
// circular arc and the two inner edges meet at a point on the core circle.
[[nodiscard]] core::BezierPath generate_droplet_rosette_bezier(
    const DropletRosetteParameters& parameters);

} // namespace rosettelab::curves
