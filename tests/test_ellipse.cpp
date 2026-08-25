#include "rosettelab/curves/ellipse.hpp"

#include <cassert>
#include <stdexcept>

int main()
{
    using namespace rosettelab::curves;

    EllipseParameters parameters;
    const auto path = generate_ellipse_bezier(parameters, parameters.bezier_tolerance);
    assert(path.closed);
    assert(path.segments.size() >= 4);
    assert(path.segments.front().start.x == path.segments.back().end.x);
    assert(path.segments.front().start.y == path.segments.back().end.y);

    const auto tighter = generate_ellipse_bezier(parameters, 0.001);
    assert(tighter.segments.size() >= path.segments.size());

    parameters.rotation_degrees = 37.0;
    assert(!generate_ellipse_bezier(parameters).segments.empty());

    bool rejected = false;
    try {
        parameters.radius_x = 0.0;
        static_cast<void>(generate_ellipse_bezier(parameters));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
}
