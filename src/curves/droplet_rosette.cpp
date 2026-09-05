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

core::Point rotate(const core::Point point, const double angle)
{
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    return {
        point.x * cosine - point.y * sine,
        point.x * sine + point.y * cosine,
    };
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
    result.segments.reserve(static_cast<std::size_t>(p.droplets) * 5);
    result.subpath_starts.reserve(static_cast<std::size_t>(p.droplets));

    const double sector = 2.0 * std::numbers::pi / static_cast<double>(p.droplets);
    const double half_span = sector * std::clamp(p.width_percent / 100.0, 0.01, 1.0) / 2.0;
    const double rotation = p.rotation_degrees * std::numbers::pi / 180.0;
    const double swirl = p.swirl_degrees * std::numbers::pi / 180.0;
    const double radial_span = p.outer_radius - p.core_radius;
    const double packing_sine = std::sin(std::numbers::pi / static_cast<double>(p.droplets));
    const double packing_radius = p.outer_radius * packing_sine / (1.0 + packing_sine);
    const double bulb_radius = std::min(
        radial_span * 0.43,
        packing_radius * p.width_percent / 100.0);
    const double bulb_distance = p.outer_radius - bulb_radius;
    const double attachment_offset = half_span * 0.72;
    const double attachment_leading = std::numbers::pi - attachment_offset;
    const double attachment_trailing = std::numbers::pi + attachment_offset;
    const double tip_angle = swirl;
    const auto tip = polar(p.core_radius, tip_angle);
    const auto tip_tangent = tangent(tip_angle);
    const core::Point bulb_centre{bulb_distance, 0.0};
    const auto point_on_bulb = [&](const double angle) {
        return core::Point{
            bulb_centre.x + bulb_radius * std::cos(angle),
            bulb_centre.y + bulb_radius * std::sin(angle),
        };
    };

    core::BezierPath base;
    base.closed = true;
    const auto leading = point_on_bulb(attachment_leading);
    const auto trailing = point_on_bulb(attachment_trailing);
    const double tail_handle = radial_span * (0.12 + 0.20 * p.roundness);
    base.segments.push_back({
        tip,
        add(tip, tip_tangent, tail_handle),
        add(leading, tangent(attachment_leading), bulb_radius * (0.30 + 0.45 * p.roundness)),
        leading,
    });

    // Traverse the long, outward side of the bulb clockwise in three arcs.
    const double arc_end = attachment_trailing - 2.0 * std::numbers::pi;
    constexpr int arc_segments = 3;
    for (int arc = 0; arc < arc_segments; ++arc) {
        const double start_angle = attachment_leading +
            (arc_end - attachment_leading) * static_cast<double>(arc) / arc_segments;
        const double end_angle = attachment_leading +
            (arc_end - attachment_leading) * static_cast<double>(arc + 1) / arc_segments;
        const auto start = point_on_bulb(start_angle);
        const auto end = point_on_bulb(end_angle);
        const double handle = 4.0 / 3.0 *
            std::tan((end_angle - start_angle) / 4.0) * bulb_radius;
        base.segments.push_back({
            start,
            add(start, tangent(start_angle), handle),
            add(end, tangent(end_angle), -handle),
            end,
        });
    }
    base.segments.push_back({
        trailing,
        add(trailing, tangent(attachment_trailing),
            -bulb_radius * (0.18 + 0.36 * p.roundness)),
        add(tip, tip_tangent, -radial_span * (0.24 + 0.34 * p.roundness)),
        tip,
    });

    for (int index = 0; index < p.droplets; ++index) {
        result.subpath_starts.push_back(result.segments.size());
        const double centre = rotation + static_cast<double>(index) * sector;
        for (const auto& segment : base.segments) {
            result.segments.push_back({
                rotate(segment.start, centre),
                rotate(segment.control1, centre),
                rotate(segment.control2, centre),
                rotate(segment.end, centre),
            });
        }
    }
    return result;
}

} // namespace rosettelab::curves
