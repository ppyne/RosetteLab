#include "rosettelab/curves/droplet_rosette.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace rosettelab::curves {
namespace {

core::Point polar(const double radius, const double angle)
{
    return {radius * std::cos(angle), radius * std::sin(angle)};
}

core::Point tangent(const double angle)
{
    return {-std::sin(angle), std::cos(angle)};
}

core::Point add(const core::Point point, const core::Point vector, const double scale)
{
    return {point.x + vector.x * scale, point.y + vector.y * scale};
}

void validate(const DropletRosetteParameters& p)
{
    if (p.droplets < 2 || p.droplets > 128) {
        throw std::invalid_argument("Droplet count must be between 2 and 128");
    }
    if (!std::isfinite(p.outer_radius) || p.outer_radius <= 0.0 ||
        !std::isfinite(p.core_radius) || p.core_radius < 0.0 ||
        p.core_radius >= p.outer_radius) {
        throw std::invalid_argument("Droplet radii must be finite and satisfy 0 <= core < outer");
    }
    if (!std::isfinite(p.swirl_degrees) || !std::isfinite(p.width_percent) ||
        p.width_percent <= 0.0 || p.width_percent > 100.0 ||
        !std::isfinite(p.roundness) || p.roundness < 0.05 || p.roundness > 1.0 ||
        !std::isfinite(p.rotation_degrees)) {
        throw std::invalid_argument("Invalid Droplet Rosette shape parameter");
    }
}

} // namespace

core::BezierPath generate_droplet_rosette_bezier(
    const DropletRosetteParameters& p)
{
    validate(p);
    core::BezierPath result;
    result.closed = true;
    result.segments.reserve(static_cast<std::size_t>(p.droplets) * 3);
    result.subpath_starts.reserve(static_cast<std::size_t>(p.droplets));

    const double sector = 2.0 * std::numbers::pi / static_cast<double>(p.droplets);
    const double half_span = sector * std::clamp(p.width_percent / 100.0, 0.01, 1.0) / 2.0;
    const double rotation = p.rotation_degrees * std::numbers::pi / 180.0;
    const double swirl = p.swirl_degrees * std::numbers::pi / 180.0;
    const double radial_span = p.outer_radius - p.core_radius;
    const double inner_handle = radial_span * (0.30 + 0.30 * p.roundness);

    for (int index = 0; index < p.droplets; ++index) {
        result.subpath_starts.push_back(result.segments.size());
        const double centre = rotation + static_cast<double>(index) * sector;
        const double start_angle = centre - half_span;
        const double end_angle = centre + half_span;
        const double tip_angle = centre + swirl;
        const auto start = polar(p.outer_radius, start_angle);
        const auto end = polar(p.outer_radius, end_angle);
        const auto tip = polar(p.core_radius, tip_angle);

        // Exact cubic approximation of the outer circular arc.
        const double arc_handle = 4.0 / 3.0 * std::tan(half_span / 2.0) * p.outer_radius;
        result.segments.push_back({
            start,
            add(start, tangent(start_angle), arc_handle),
            add(end, tangent(end_angle), -arc_handle),
            end,
        });

        // The inner boundaries share a tangent at the tip. Changing the swirl
        // angle displaces that tangent and creates the characteristic rotation.
        const auto tip_tangent = tangent(tip_angle);
        const auto inward_end = polar(1.0, end_angle + std::numbers::pi);
        const auto outward_start = polar(1.0, start_angle);
        result.segments.push_back({
            end,
            add(end, inward_end, radial_span * (0.35 + 0.35 * p.roundness)),
            add(tip, tip_tangent, -inner_handle),
            tip,
        });
        result.segments.push_back({
            tip,
            add(tip, tip_tangent, inner_handle),
            add(start, outward_start, -radial_span * (0.35 + 0.35 * p.roundness)),
            start,
        });
    }
    return result;
}

} // namespace rosettelab::curves
