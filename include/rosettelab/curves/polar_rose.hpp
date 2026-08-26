#pragma once

#include "rosettelab/core/geometry.hpp"

#include <cstddef>

namespace rosettelab::curves {

enum class PolarKMode {
    Decimal,
    Fraction,
};

struct PolarRoseParameters {
    double radius{100.0};
    PolarKMode k_mode{PolarKMode::Decimal};
    double k{7.0};
    int numerator{7};
    int denominator{1};
    double phase_degrees{0.0};
    double rotation_degrees{0.0};
    double bezier_tolerance{0.05};
    std::size_t samples{720};

    friend constexpr bool operator==(const PolarRoseParameters&, const PolarRoseParameters&) = default;
};

[[nodiscard]] double effective_k(const PolarRoseParameters& parameters);
[[nodiscard]] double polar_rose_period(const PolarRoseParameters& parameters);
[[nodiscard]] bool polar_rose_is_closed(const PolarRoseParameters& parameters);

[[nodiscard]] core::Polyline generate_polar_rose(const PolarRoseParameters& parameters);
[[nodiscard]] core::BezierPath generate_polar_rose_bezier(
    const PolarRoseParameters& parameters,
    double tolerance = 0.05);

} // namespace rosettelab::curves
