#include "rosettelab/curves/lissajous.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <numbers>
#include <stdexcept>

namespace rosettelab::curves {
namespace {

double radians(const double degrees) { return degrees * std::numbers::pi / 180.0; }

core::Point rotate(const core::Point p, const double angle)
{
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {p.x * c - p.y * s, p.x * s + p.y * c};
}

core::Point point_at(const LissajousParameters& p, const double t)
{
    return rotate({
        p.amplitude_x * std::sin(p.frequency_x * t + radians(p.phase_x_degrees)),
        p.amplitude_y * std::sin(p.frequency_y * t + radians(p.phase_y_degrees)),
    }, radians(p.rotation_degrees));
}

core::Point derivative_at(const LissajousParameters& p, const double t)
{
    return rotate({
        p.amplitude_x * p.frequency_x * std::cos(p.frequency_x * t + radians(p.phase_x_degrees)),
        p.amplitude_y * p.frequency_y * std::cos(p.frequency_y * t + radians(p.phase_y_degrees)),
    }, radians(p.rotation_degrees));
}

core::Point cubic_at(const core::CubicBezier& c, const double u)
{
    const double v = 1.0 - u;
    return {
        v*v*v*c.start.x + 3*v*v*u*c.control1.x + 3*v*u*u*c.control2.x + u*u*u*c.end.x,
        v*v*v*c.start.y + 3*v*v*u*c.control1.y + 3*v*u*u*c.control2.y + u*u*u*c.end.y,
    };
}

core::CubicBezier segment_at(const LissajousParameters& p, const double a, const double b)
{
    const auto p0 = point_at(p, a);
    const auto p3 = point_at(p, b);
    const auto d0 = derivative_at(p, a);
    const auto d1 = derivative_at(p, b);
    const double scale = (b - a) / 3.0;
    return {p0, {p0.x + scale*d0.x, p0.y + scale*d0.y},
                {p3.x - scale*d1.x, p3.y - scale*d1.y}, p3};
}

double error(const LissajousParameters& p, const core::CubicBezier& c, const double a, const double b)
{
    double result = 0.0;
    for (const double u : {0.25, 0.5, 0.75}) {
        const auto exact = point_at(p, a + (b-a)*u);
        const auto approximate = cubic_at(c, u);
        result = std::max(result, std::hypot(exact.x-approximate.x, exact.y-approximate.y));
    }
    return result;
}

void validate(const LissajousParameters& p, const double tolerance)
{
    if (!std::isfinite(p.amplitude_x) || p.amplitude_x <= 0.0 ||
        !std::isfinite(p.amplitude_y) || p.amplitude_y <= 0.0 ||
        p.frequency_x <= 0 || p.frequency_y <= 0 ||
        !std::isfinite(p.phase_x_degrees) || !std::isfinite(p.phase_y_degrees) ||
        !std::isfinite(p.rotation_degrees) || !std::isfinite(tolerance) || tolerance <= 0.0) {
        throw std::invalid_argument("Lissajous parameters must be finite and positive where required");
    }
}

} // namespace

core::BezierPath generate_lissajous_bezier(const LissajousParameters& p, const double tolerance)
{
    validate(p, tolerance);
    core::BezierPath result;
    result.closed = true;
    const int divisor = std::gcd(p.frequency_x, p.frequency_y);
    const double end = 2.0 * std::numbers::pi / divisor;
    const std::size_t intervals = static_cast<std::size_t>(
        8 * std::max(p.frequency_x, p.frequency_y) / divisor);
    constexpr std::size_t maximum_segments = 100'000;
    constexpr int maximum_depth = 18;
    const auto append = [&](auto&& self, const double a, const double b, const int depth) -> void {
        const auto segment = segment_at(p, a, b);
        if (depth >= maximum_depth || error(p, segment, a, b) <= tolerance) {
            if (result.segments.size() >= maximum_segments) {
                throw std::length_error("Lissajous curve exceeds the supported Bezier segment count");
            }
            result.segments.push_back(segment);
            return;
        }
        const double middle = (a+b)/2.0;
        self(self, a, middle, depth+1);
        self(self, middle, b, depth+1);
    };
    for (std::size_t i=0; i<intervals; ++i) {
        append(append, end*i/intervals, end*(i+1)/intervals, 0);
    }
    result.segments.back().end = result.segments.front().start;
    return result;
}

} // namespace rosettelab::curves
