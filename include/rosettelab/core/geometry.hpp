#pragma once

#include <cstddef>
#include <vector>

namespace rosettelab::core {

struct Point {
    double x{};
    double y{};

    friend constexpr bool operator==(const Point&, const Point&) = default;
};

struct Polyline {
    std::vector<Point> points;
    bool closed{false};
};

struct CubicBezier {
    Point start;
    Point control1;
    Point control2;
    Point end;
};

struct BezierPath {
    std::vector<CubicBezier> segments;
    // Segment indices that begin a new subpath. Empty means a single subpath
    // beginning at segment zero. This permits one layer to contain several
    // independently closed contours while retaining a single SVG/PDF path.
    std::vector<std::size_t> subpath_starts;
    bool closed{false};
};

} // namespace rosettelab::core
