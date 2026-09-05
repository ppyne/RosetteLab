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
            require(path.segments.size() == static_cast<std::size_t>(count * 3),
                    "each droplet must contain three cubic segments");
            require(path.subpath_starts.size() == static_cast<std::size_t>(count),
                    "each droplet must begin an independent subpath");
            for (int i = 0; i < count; ++i) {
                const auto first = static_cast<std::size_t>(i * 3);
                require(path.subpath_starts[static_cast<std::size_t>(i)] == first,
                        "subpath starts must be deterministic");
                require(std::hypot(
                    path.segments[first].start.x - path.segments[first + 2].end.x,
                    path.segments[first].start.y - path.segments[first + 2].end.y) < 1e-9,
                    "every droplet must close geometrically");
            }
        }

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
