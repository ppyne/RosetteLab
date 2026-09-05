#include "rosettelab/curves/droplet_rosette.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
void require(const bool condition, const std::string_view message)
{
    if (!condition) throw std::runtime_error(std::string(message));
}
}

int main()
{
    try {
        for (const int count : {2, 3, 5, 8}) {
            rosettelab::curves::DropletRosetteParameters parameters;
            parameters.droplets = count;
            const auto path = rosettelab::curves::generate_droplet_rosette_bezier(parameters);
            require(path.closed, "Droplet Rosette must be closed");
            const std::size_t segments_per_droplet = count == 3 ? 7 : 6;
            require(path.segments.size() == static_cast<std::size_t>(count) * segments_per_droplet,
                    "each droplet must contain the expected cubic segments");
            require(path.subpath_starts.size() == static_cast<std::size_t>(count),
                    "each droplet must begin an independent subpath");
            for (int i = 0; i < count; ++i) {
                const auto first = static_cast<std::size_t>(i) * segments_per_droplet;
                require(path.subpath_starts[static_cast<std::size_t>(i)] == first,
                        "subpath starts must be deterministic");
                require(std::hypot(
                    path.segments[first].start.x - path.segments[first + segments_per_droplet - 1].end.x,
                    path.segments[first].start.y - path.segments[first + segments_per_droplet - 1].end.y) < 1e-9,
                    "every droplet must close geometrically");
            }
        }

        rosettelab::curves::DropletRosetteParameters pair;
        pair.droplets = 2;
        pair.outer_radius = 100.0;
        pair.rotation_degrees = 0.0;
        const auto taijitu = rosettelab::curves::generate_droplet_rosette_bezier(pair);
        require(taijitu.subpath_starts == std::vector<std::size_t>{0, 6},
                "Taijitu must contain two six-segment contours");
        require(std::abs(std::hypot(taijitu.segments.front().start.x,
                                    taijitu.segments.front().start.y) - 100.0) < 1e-9,
                "Taijitu contour must start on its outer circle");

        bool rejected = false;
        try {
            rosettelab::curves::DropletRosetteParameters invalid;
            invalid.droplets = 1;
            static_cast<void>(rosettelab::curves::generate_droplet_rosette_bezier(invalid));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "invalid droplet counts must be rejected");
        std::cout << "All Droplet Rosette tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
