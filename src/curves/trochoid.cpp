#include "rosettelab/curves/trochoid.hpp"

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

double frequency(const TrochoidKind kind, const TrochoidParameters& parameters)
{
    return kind == TrochoidKind::Hypotrochoid
        ? (parameters.fixed_radius - parameters.rolling_radius) / parameters.rolling_radius
        : (parameters.fixed_radius + parameters.rolling_radius) / parameters.rolling_radius;
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

core::Point point_at(
    const TrochoidKind kind,
    const TrochoidParameters& parameters,
    const double t)
{
    const double base = kind == TrochoidKind::Hypotrochoid
        ? parameters.fixed_radius - parameters.rolling_radius
        : parameters.fixed_radius + parameters.rolling_radius;
    const double x_sign = kind == TrochoidKind::Hypotrochoid ? 1.0 : -1.0;
    const double secondary = frequency(kind, parameters) * t;
    return rotate({
        base * std::cos(t) + x_sign * parameters.pen_offset * std::cos(secondary),
        base * std::sin(t) - parameters.pen_offset * std::sin(secondary),
    }, radians(parameters.rotation_degrees));
}

core::Point derivative_at(
    const TrochoidKind kind,
    const TrochoidParameters& parameters,
    const double t)
{
    const double base = kind == TrochoidKind::Hypotrochoid
        ? parameters.fixed_radius - parameters.rolling_radius
        : parameters.fixed_radius + parameters.rolling_radius;
    const double x_sign = kind == TrochoidKind::Hypotrochoid ? 1.0 : -1.0;
    const double omega = frequency(kind, parameters);
    const double secondary = omega * t;
    return rotate({
        -base * std::sin(t) - x_sign * parameters.pen_offset * omega * std::sin(secondary),
        base * std::cos(t) - parameters.pen_offset * omega * std::cos(secondary),
    }, radians(parameters.rotation_degrees));
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

core::CubicBezier hermite_segment(
    const TrochoidKind kind,
    const TrochoidParameters& parameters,
    const double start,
    const double end)
{
    const auto p0 = point_at(kind, parameters, start);
    const auto p3 = point_at(kind, parameters, end);
    const auto d0 = derivative_at(kind, parameters, start);
    const auto d1 = derivative_at(kind, parameters, end);
    const double scale = (end - start) / 3.0;
    return {
        p0,
        {p0.x + scale * d0.x, p0.y + scale * d0.y},
        {p3.x - scale * d1.x, p3.y - scale * d1.y},
        p3,
    };
}

double segment_error(
    const TrochoidKind kind,
    const TrochoidParameters& parameters,
    const core::CubicBezier& segment,
    const double start,
    const double end)
{
    double maximum = 0.0;
    for (const double u : {0.25, 0.5, 0.75}) {
        const auto exact = point_at(kind, parameters, start + (end - start) * u);
        const auto approximate = cubic_at(segment, u);
        maximum = std::max(maximum, std::hypot(
            exact.x - approximate.x, exact.y - approximate.y));
    }
    return maximum;
}

int rational_denominator(const double ratio)
{
    constexpr int maximum_denominator = 10'000;
    constexpr double tolerance = 1e-10;
    for (int denominator = 1; denominator <= maximum_denominator; ++denominator) {
        const double numerator = std::round(ratio * denominator);
        if (std::abs(ratio - numerator / denominator) <= tolerance) {
            return denominator;
        }
    }
    throw std::invalid_argument(
        "Complete trochoid trace requires a rational R/r ratio with denominator at most 10000");
}

void validate(const TrochoidParameters& parameters, const double tolerance)
{
    if (!std::isfinite(parameters.fixed_radius) || parameters.fixed_radius <= 0.0 ||
        !std::isfinite(parameters.rolling_radius) || parameters.rolling_radius <= 0.0 ||
        !std::isfinite(parameters.pen_offset) || parameters.pen_offset < 0.0) {
        throw std::invalid_argument("Trochoid radii must be finite and positive and d must be non-negative");
    }
    if (!std::isfinite(parameters.rotation_degrees) ||
        !std::isfinite(parameters.turns) || parameters.turns <= 0.0 ||
        !std::isfinite(tolerance) || tolerance <= 0.0) {
        throw std::invalid_argument("Trochoid rotation, turns, and tolerance must be finite and valid");
    }
}

} // namespace

double trochoid_trace_end(
    const TrochoidKind,
    const TrochoidParameters& parameters)
{
    validate(parameters, parameters.bezier_tolerance);
    const double turns = parameters.trace_mode == TraceMode::Complete
        ? static_cast<double>(rational_denominator(
              parameters.fixed_radius / parameters.rolling_radius))
        : parameters.turns;
    return 2.0 * std::numbers::pi * turns;
}

core::BezierPath generate_trochoid_bezier(
    const TrochoidKind kind,
    const TrochoidParameters& parameters,
    const double tolerance)
{
    validate(parameters, tolerance);
    const double end_time = trochoid_trace_end(kind, parameters);
    core::BezierPath result;
    result.closed = parameters.trace_mode == TraceMode::Complete || parameters.close_limited_path;

    constexpr std::size_t maximum_segments = 250'000;
    constexpr int maximum_depth = 20;
    const double turns = end_time / (2.0 * std::numbers::pi);
    const auto initial_intervals = std::clamp<std::size_t>(
        static_cast<std::size_t>(std::ceil(
            turns * std::max(1.0, std::abs(frequency(kind, parameters))) * 8.0)),
        8, 16'384);
    result.segments.reserve(initial_intervals);

    const auto append_interval = [&](auto&& self, const double start, const double end, const int depth) -> void {
        const auto segment = hermite_segment(kind, parameters, start, end);
        if (depth >= maximum_depth ||
            segment_error(kind, parameters, segment, start, end) <= tolerance) {
            if (result.segments.size() >= maximum_segments) {
                throw std::length_error("Trochoid exceeds the supported Bezier segment count");
            }
            result.segments.push_back(segment);
            return;
        }
        const double middle = (start + end) / 2.0;
        self(self, start, middle, depth + 1);
        self(self, middle, end, depth + 1);
    };

    for (std::size_t index = 0; index < initial_intervals; ++index) {
        const double start = end_time * static_cast<double>(index) /
                             static_cast<double>(initial_intervals);
        const double end = end_time * static_cast<double>(index + 1) /
                           static_cast<double>(initial_intervals);
        append_interval(append_interval, start, end, 0);
    }
    if (parameters.trace_mode == TraceMode::Complete && !result.segments.empty()) {
        result.segments.back().end = result.segments.front().start;
    }
    return result;
}

} // namespace rosettelab::curves
