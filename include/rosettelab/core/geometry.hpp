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

} // namespace rosettelab::core

