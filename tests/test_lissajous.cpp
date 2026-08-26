#include "rosettelab/curves/lissajous.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>

int main()
{
    using namespace rosettelab::curves;
    LissajousParameters parameters;
    const auto path = generate_lissajous_bezier(parameters);
    assert(path.closed && !path.segments.empty());
    assert(std::hypot(path.segments.front().start.x - path.segments.back().end.x,
                      path.segments.front().start.y - path.segments.back().end.y) < 1e-9);
    parameters.frequency_x = 5;
    parameters.frequency_y = 4;
    parameters.phase_y_degrees = 117.0;
    assert(!generate_lissajous_bezier(parameters).segments.empty());
    try {
        parameters.frequency_x = 0;
        static_cast<void>(generate_lissajous_bezier(parameters));
        assert(false);
    } catch (const std::invalid_argument&) {
    }
}
