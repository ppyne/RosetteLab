#include "rosettelab/curves/harmonograph.hpp"

#include <cassert>
#include <stdexcept>

int main()
{
    using namespace rosettelab::curves;
    HarmonographParameters parameters;
    const auto path=generate_harmonograph_bezier(parameters);
    assert(!path.closed);
    assert(!path.segments.empty());
    assert(path.segments.front().start.x != path.segments.back().end.x ||
           path.segments.front().start.y != path.segments.back().end.y);
    parameters.damping_x=-0.1;
    try { static_cast<void>(generate_harmonograph_bezier(parameters)); }
    catch (const std::invalid_argument&) { return 0; }
    return 1;
}
