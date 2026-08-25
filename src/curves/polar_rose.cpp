#include "rosettelab/curves/polar_rose.hpp"

#include <cmath>
#include <functional>
#include <numeric>
#include <numbers>
#include <stdexcept>

namespace rosettelab::curves {
namespace {

constexpr std::size_t minimum_samples = 16;
constexpr std::size_t maximum_samples = 250'000;

double radians(const double degrees)
{
    return degrees * std::numbers::pi / 180.0;
}

core::Point point_at(const PolarRoseParameters& parameters, const double theta)
{
    const double phase = radians(parameters.phase_degrees);
    const double rotation = radians(parameters.rotation_degrees);
    const double radius = parameters.radius * std::cos(effective_k(parameters) * theta + phase);
    const double angle = theta + rotation;
    return {radius * std::cos(angle), radius * std::sin(angle)};
}

core::Point derivative_at(const PolarRoseParameters& parameters, const double theta)
{
    const double phase = radians(parameters.phase_degrees);
    const double rotation = radians(parameters.rotation_degrees);
    const double k = effective_k(parameters);
    const double argument = k * theta + phase;
    const double radius = parameters.radius * std::cos(argument);
    const double radius_derivative = -parameters.radius * k * std::sin(argument);
    const double angle = theta + rotation;
    return {
        radius_derivative * std::cos(angle) - radius * std::sin(angle),
        radius_derivative * std::sin(angle) + radius * std::cos(angle),
    };
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

double distance(const core::Point& left, const core::Point& right)
{
    return std::hypot(left.x - right.x, left.y - right.y);
}

core::CubicBezier hermite_segment(
    const PolarRoseParameters& parameters,
    const double start,
    const double end)
{
    const auto p0 = point_at(parameters, start);
    const auto p3 = point_at(parameters, end);
    const auto d0 = derivative_at(parameters, start);
    const auto d1 = derivative_at(parameters, end);
    const double scale = (end - start) / 3.0;
    return {
        p0,
        {p0.x + scale * d0.x, p0.y + scale * d0.y},
        {p3.x - scale * d1.x, p3.y - scale * d1.y},
        p3,
    };
}

double segment_error(
    const PolarRoseParameters& parameters,
    const core::CubicBezier& segment,
    const double start,
    const double end)
{
    double maximum = 0.0;
    for (const double u : {0.25, 0.5, 0.75}) {
        const auto exact = point_at(parameters, start + (end - start) * u);
        maximum = std::max(maximum, distance(exact, cubic_at(segment, u)));
    }
    return maximum;
}

void validate(const PolarRoseParameters& parameters)
{
    if (!std::isfinite(parameters.radius) || parameters.radius <= 0.0) {
        throw std::invalid_argument("Polar rose radius must be finite and positive");
    }
    if (parameters.k_mode == PolarKMode::Decimal &&
        (!std::isfinite(parameters.k) || parameters.k == 0.0)) {
        throw std::invalid_argument("Decimal polar rose k must be finite and non-zero");
    }
    if (parameters.k_mode == PolarKMode::Fraction &&
        (parameters.numerator == 0 || parameters.denominator == 0)) {
        throw std::invalid_argument("Fractional polar rose numerator and denominator must be non-zero");
    }
    if (!std::isfinite(parameters.phase_degrees) ||
        !std::isfinite(parameters.rotation_degrees)) {
        throw std::invalid_argument("Polar rose angles must be finite");
    }
    if (parameters.samples < minimum_samples || parameters.samples > maximum_samples) {
        throw std::invalid_argument("Polar rose sample count is outside the supported range");
    }
}

} // namespace

double effective_k(const PolarRoseParameters& parameters)
{
    if (parameters.k_mode == PolarKMode::Fraction) {
        if (parameters.denominator == 0) {
            throw std::invalid_argument("Polar rose denominator must be non-zero");
        }
        return static_cast<double>(parameters.numerator) /
               static_cast<double>(parameters.denominator);
    }
    return parameters.k;
}

double polar_rose_period(const PolarRoseParameters& parameters)
{
    if (parameters.k_mode == PolarKMode::Decimal) {
        return 2.0 * std::numbers::pi;
    }

    const auto numerator = std::abs(parameters.numerator);
    const auto denominator = std::abs(parameters.denominator);
    if (numerator == 0 || denominator == 0) {
        throw std::invalid_argument("Polar rose fraction must contain non-zero integers");
    }
    const auto divisor = std::gcd(numerator, denominator);
    const auto reduced_numerator = numerator / divisor;
    const auto reduced_denominator = denominator / divisor;
    const bool both_odd = reduced_numerator % 2 != 0 && reduced_denominator % 2 != 0;
    return (both_odd ? 1.0 : 2.0) * static_cast<double>(reduced_denominator) *
           std::numbers::pi;
}

bool polar_rose_is_closed(const PolarRoseParameters& parameters)
{
    if (parameters.k_mode == PolarKMode::Fraction) {
        return parameters.numerator != 0 && parameters.denominator != 0;
    }
    return std::isfinite(parameters.k) &&
           std::abs(parameters.k - std::round(parameters.k)) < 1e-12;
}

core::Polyline generate_polar_rose(const PolarRoseParameters& parameters)
{
    validate(parameters);

    core::Polyline result;
    result.closed = polar_rose_is_closed(parameters);
    result.points.reserve(parameters.samples + 1);

    const double phase = radians(parameters.phase_degrees);
    const double rotation = radians(parameters.rotation_degrees);
    const double full_turn = polar_rose_period(parameters);

    for (std::size_t index = 0; index <= parameters.samples; ++index) {
        const double theta = full_turn * static_cast<double>(index) /
                             static_cast<double>(parameters.samples);
        const double radius = parameters.radius * std::cos(parameters.k * theta + phase);
        const double angle = theta + rotation;
        result.points.push_back({radius * std::cos(angle), radius * std::sin(angle)});
    }

    if (result.closed) {
        // Enforce exact topological closure despite floating-point trigonometry.
        result.points.back() = result.points.front();
    }
    return result;
}

core::BezierPath generate_polar_rose_bezier(
    const PolarRoseParameters& parameters,
    const double tolerance)
{
    validate(parameters);
    if (!std::isfinite(tolerance) || tolerance <= 0.0) {
        throw std::invalid_argument("Bezier tolerance must be finite and positive");
    }

    core::BezierPath result;
    result.closed = polar_rose_is_closed(parameters);

    constexpr std::size_t maximum_segments = 250'000;
    constexpr int maximum_depth = 20;
    const auto initial_intervals = std::clamp<std::size_t>(
        static_cast<std::size_t>(std::ceil(std::max(
            polar_rose_period(parameters) / (2.0 * std::numbers::pi),
            std::abs(effective_k(parameters)) * polar_rose_period(parameters) /
                (2.0 * std::numbers::pi)) * 8.0)),
        8, 4096);
    result.segments.reserve(initial_intervals);

    const auto append_interval = [&](auto&& self, const double start, const double end, const int depth) -> void {
        const auto segment = hermite_segment(parameters, start, end);
        if (depth >= maximum_depth || segment_error(parameters, segment, start, end) <= tolerance) {
            if (result.segments.size() >= maximum_segments) {
                throw std::length_error("Bezier path exceeds the supported segment count");
            }
            result.segments.push_back(segment);
            return;
        }

        const double middle = (start + end) / 2.0;
        self(self, start, middle, depth + 1);
        self(self, middle, end, depth + 1);
    };

    const double full_turn = polar_rose_period(parameters);
    for (std::size_t index = 0; index < initial_intervals; ++index) {
        const double start = full_turn * static_cast<double>(index) /
                             static_cast<double>(initial_intervals);
        const double end = full_turn * static_cast<double>(index + 1) /
                           static_cast<double>(initial_intervals);
        append_interval(append_interval, start, end, 0);
    }

    if (result.closed && !result.segments.empty()) {
        result.segments.back().end = result.segments.front().start;
    }
    return result;
}

} // namespace rosettelab::curves
