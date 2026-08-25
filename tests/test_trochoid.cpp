#include "rosettelab/curves/trochoid.hpp"

#include <cassert>
#include <cmath>
#include <numbers>

int main()
{
    using namespace rosettelab::curves;

    TrochoidParameters parameters;
    assert(std::abs(trochoid_trace_end(TrochoidKind::Hypotrochoid, parameters) -
                    6.0 * std::numbers::pi) < 1e-12);
    const auto hypo = generate_trochoid_bezier(TrochoidKind::Hypotrochoid, parameters);
    assert(hypo.closed && !hypo.segments.empty());
    assert(hypo.segments.front().start.x == hypo.segments.back().end.x);
    assert(hypo.segments.front().start.y == hypo.segments.back().end.y);

    const auto epi = generate_trochoid_bezier(TrochoidKind::Epitrochoid, parameters);
    assert(epi.closed && !epi.segments.empty());

    parameters.fixed_radius = 32.0;
    parameters.rolling_radius = 63.0;
    parameters.pen_offset = 44.5;
    parameters.trace_mode = TraceMode::Limited;
    parameters.turns = 2.0;
    const auto limited = generate_trochoid_bezier(TrochoidKind::Epitrochoid, parameters);
    assert(!limited.closed);
    assert(std::abs(trochoid_trace_end(TrochoidKind::Epitrochoid, parameters) -
                    4.0 * std::numbers::pi) < 1e-12);

    parameters.close_limited_path = true;
    assert(generate_trochoid_bezier(TrochoidKind::Epitrochoid, parameters).closed);
}
