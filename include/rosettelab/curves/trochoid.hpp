#pragma once

#include "rosettelab/core/geometry.hpp"

namespace rosettelab::curves {

enum class TrochoidKind {
    Hypotrochoid,
    Epitrochoid,
};

enum class TraceMode {
    Complete,
    Limited,
};

struct TrochoidParameters {
    double fixed_radius{105.0};
    double rolling_radius{45.0};
    double pen_offset{30.0};
    double rotation_degrees{0.0};
    TraceMode trace_mode{TraceMode::Complete};
    double turns{2.0};
    bool close_limited_path{false};
    double bezier_tolerance{0.05};
};

[[nodiscard]] double trochoid_trace_end(
    TrochoidKind kind,
    const TrochoidParameters& parameters);

[[nodiscard]] core::BezierPath generate_trochoid_bezier(
    TrochoidKind kind,
    const TrochoidParameters& parameters,
    double tolerance = 0.05);

} // namespace rosettelab::curves
