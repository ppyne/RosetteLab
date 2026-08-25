#pragma once

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
    bool closed{false};
};

} // namespace rosettelab::core
