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

core::Point add(const core::Point point, const core::Point vector, const double factor)
{
    return {point.x + vector.x * factor, point.y + vector.y * factor};
}

void append_circular_arc(
    core::BezierPath& path,
    const core::Point centre,
    const double radius,
    const double start_angle,
    const double sweep)
{
    const int pieces = std::max(1, static_cast<int>(
        std::ceil(std::abs(sweep) / (std::numbers::pi / 2.0))));
    for (int piece = 0; piece < pieces; ++piece) {
        const double start = start_angle + sweep * static_cast<double>(piece) / pieces;
        const double end = start_angle + sweep * static_cast<double>(piece + 1) / pieces;
        const auto p0 = add(centre, polar(1.0, start), radius);
        const auto p3 = add(centre, polar(1.0, end), radius);
        const double handle = 4.0 / 3.0 * std::tan((end - start) / 4.0) * radius;
        path.segments.push_back({
            p0,
            add(p0, tangent(start), handle),
            add(p3, tangent(end), -handle),
            p3,
        });
    }
}

void validate(const DropletRosetteParameters& p)
{
    if (p.droplets < 2 || p.droplets > 128) {
        throw std::invalid_argument("Droplet count must be between 2 and 128");
    }
    if (!std::isfinite(p.outer_radius) || p.outer_radius <= 0.0 ||
        !std::isfinite(p.core_radius) || !std::isfinite(p.swirl_degrees) ||
        !std::isfinite(p.width_percent) || !std::isfinite(p.roundness) ||
        !std::isfinite(p.rotation_degrees)) {
        throw std::invalid_argument("Droplet Rosette parameters must be finite");
    }
}

} // namespace

core::BezierPath generate_droplet_rosette_bezier(
    const DropletRosetteParameters& p)
{
    validate(p);

    core::BezierPath result;
    result.closed = true;
    const double count = static_cast<double>(p.droplets);
    const double sector = 2.0 * std::numbers::pi / count;
    const double half_sector = std::numbers::pi / count;
    const double sine = std::sin(half_sector);
    const double inner_radius = p.outer_radius * sine / (1.0 + sine);
    const double centre_radius = p.outer_radius - inner_radius;
    const double rotation = p.rotation_degrees * std::numbers::pi / 180.0;

    // Each contour consists of a short outer arc, the major arc of the current
    // packed circle, and the minor arc of the previous packed circle. The two
    // inner arcs share a tangent at their common kissing point.
    const double major_sweep = 3.0 * std::numbers::pi / 2.0 - half_sector;
    const double minor_sweep = -(std::numbers::pi / 2.0 + half_sector);

    for (int index = 0; index < p.droplets; ++index) {
        result.subpath_starts.push_back(result.segments.size());
        const double current_angle = rotation + static_cast<double>(index) * sector;
        const double previous_angle = current_angle - sector;
        const auto current_centre = polar(centre_radius, current_angle);
        const auto previous_centre = polar(centre_radius, previous_angle);

        append_circular_arc(
            result, {0.0, 0.0}, p.outer_radius, previous_angle, sector);
        append_circular_arc(
            result, current_centre, inner_radius, current_angle, major_sweep);
        append_circular_arc(
            result, previous_centre, inner_radius,
            previous_angle + std::numbers::pi / 2.0 + half_sector,
            minor_sweep);
    }
    return result;
}

} // namespace rosettelab::curves
