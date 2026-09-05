#pragma once

#include <cstddef>
#include <utility>
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

[[nodiscard]] inline std::vector<BezierPath> split_subpaths(const BezierPath& path)
{
    if (path.segments.empty()) return {};
    std::vector<std::size_t> starts = path.subpath_starts;
    if (starts.empty() || starts.front() != 0) starts.insert(starts.begin(), 0);
    starts.push_back(path.segments.size());
    std::vector<BezierPath> result;
    result.reserve(starts.size() - 1);
    for (std::size_t index = 0; index + 1 < starts.size(); ++index) {
        BezierPath part;
        part.closed = path.closed;
        part.segments.assign(
            path.segments.begin() + static_cast<std::ptrdiff_t>(starts[index]),
            path.segments.begin() + static_cast<std::ptrdiff_t>(starts[index + 1]));
        result.push_back(std::move(part));
    }
    return result;
}

} // namespace rosettelab::core
