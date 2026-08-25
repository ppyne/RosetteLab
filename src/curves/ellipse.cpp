#include "rosettelab/curves/ellipse.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace rosettelab::curves {
namespace {

double radians(const double degrees)
{
    return degrees * std::numbers::pi / 180.0;
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

core::Point point_at(const EllipseParameters& parameters, const double angle)
{
    return rotate(
        {parameters.radius_x * std::cos(angle), parameters.radius_y * std::sin(angle)},
        radians(parameters.rotation_degrees));
}

core::Point derivative_at(const EllipseParameters& parameters, const double angle)
{
    return rotate(
        {-parameters.radius_x * std::sin(angle), parameters.radius_y * std::cos(angle)},
        radians(parameters.rotation_degrees));
}

core::Point cubic_at(const core::CubicBezier& curve, const double u)
{
    const double inverse = 1.0 - u;
    const double w0 = inverse * inverse * inverse;
    const double w1 = 3.0 * inverse * inverse * u;
    const double w2 = 3.0 * inverse * u * u;
    const double w3 = u * u * u;
    return {
        w0 * curve.start.x + w1 * curve.control1.x + w2 * curve.control2.x + w3 * curve.end.x,
        w0 * curve.start.y + w1 * curve.control1.y + w2 * curve.control2.y + w3 * curve.end.y,
    };
}

core::CubicBezier arc_segment(
    const EllipseParameters& parameters,
    const double start,
    const double end)
{
    const auto p0 = point_at(parameters, start);
    const auto p3 = point_at(parameters, end);
    const auto d0 = derivative_at(parameters, start);
    const auto d1 = derivative_at(parameters, end);
    const double alpha = 4.0 / 3.0 * std::tan((end - start) / 4.0);
    return {
        p0,
        {p0.x + alpha * d0.x, p0.y + alpha * d0.y},
        {p3.x - alpha * d1.x, p3.y - alpha * d1.y},
        p3,
    };
}

double segment_error(
    const EllipseParameters& parameters,
    const core::CubicBezier& segment,
    const double start,
    const double end)
{
    double maximum = 0.0;
    for (const double u : {0.25, 0.5, 0.75}) {
        const auto exact = point_at(parameters, start + (end - start) * u);
        const auto approximate = cubic_at(segment, u);
        maximum = std::max(maximum, std::hypot(
            exact.x - approximate.x, exact.y - approximate.y));
    }
    return maximum;
}

void validate(const EllipseParameters& parameters, const double tolerance)
{
    if (!std::isfinite(parameters.radius_x) || parameters.radius_x <= 0.0 ||
        !std::isfinite(parameters.radius_y) || parameters.radius_y <= 0.0) {
        throw std::invalid_argument("Ellipse radii must be finite and positive");
    }
    if (!std::isfinite(parameters.rotation_degrees)) {
        throw std::invalid_argument("Ellipse rotation must be finite");
    }
    if (!std::isfinite(tolerance) || tolerance <= 0.0) {
        throw std::invalid_argument("Bezier tolerance must be finite and positive");
    }
}

} // namespace

core::BezierPath generate_ellipse_bezier(
    const EllipseParameters& parameters,
    const double tolerance)
{
    validate(parameters, tolerance);

    core::BezierPath result;
    result.closed = true;
    constexpr std::size_t maximum_segments = 4096;
    constexpr int maximum_depth = 10;

    const auto append_interval = [&](auto&& self, const double start, const double end, const int depth) -> void {
        const auto segment = arc_segment(parameters, start, end);
        if (depth >= maximum_depth || segment_error(parameters, segment, start, end) <= tolerance) {
            if (result.segments.size() >= maximum_segments) {
                throw std::length_error("Ellipse exceeds the supported Bezier segment count");
            }
            result.segments.push_back(segment);
            return;
        }
        const double middle = (start + end) / 2.0;
        self(self, start, middle, depth + 1);
        self(self, middle, end, depth + 1);
    };

    for (int quadrant = 0; quadrant < 4; ++quadrant) {
        const double start = static_cast<double>(quadrant) * std::numbers::pi / 2.0;
        const double end = static_cast<double>(quadrant + 1) * std::numbers::pi / 2.0;
        append_interval(append_interval, start, end, 0);
    }
    result.segments.back().end = result.segments.front().start;
    return result;
}

} // namespace rosettelab::curves
