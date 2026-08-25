#include "rosettelab/curves/polar_rose.hpp"

#include <cmath>
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

void validate(const PolarRoseParameters& parameters)
{
    if (!std::isfinite(parameters.radius) || parameters.radius <= 0.0) {
        throw std::invalid_argument("Polar rose radius must be finite and positive");
    }
    if (!std::isfinite(parameters.k) || parameters.k == 0.0) {
        throw std::invalid_argument("Polar rose k must be finite and non-zero");
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

core::Polyline generate_polar_rose(const PolarRoseParameters& parameters)
{
    validate(parameters);

    core::Polyline result;
    result.closed = true;
    result.points.reserve(parameters.samples + 1);

    const double phase = radians(parameters.phase_degrees);
    const double rotation = radians(parameters.rotation_degrees);
    const double full_turn = 2.0 * std::numbers::pi;

    for (std::size_t index = 0; index <= parameters.samples; ++index) {
        const double theta = full_turn * static_cast<double>(index) /
                             static_cast<double>(parameters.samples);
        const double radius = parameters.radius * std::cos(parameters.k * theta + phase);
        const double angle = theta + rotation;
        result.points.push_back({radius * std::cos(angle), radius * std::sin(angle)});
    }

    // Enforce exact topological closure despite floating-point trigonometry.
    result.points.back() = result.points.front();
    return result;
}

} // namespace rosettelab::curves

