#include "rosettelab/curves/polar_rose.hpp"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void require(const bool condition, const std::string_view message)
{
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

void test_default_curve_is_closed()
{
    const auto curve = rosettelab::curves::generate_polar_rose({});
    require(curve.closed, "default rose should be marked closed");
    require(curve.points.size() == 721, "default rose should include its closing point");
    require(curve.points.front() == curve.points.back(), "closing point should be exact");
}

void test_radius_bounds()
{
    rosettelab::curves::PolarRoseParameters parameters;
    parameters.radius = 42.0;
    parameters.k = 4.0;
    const auto curve = rosettelab::curves::generate_polar_rose(parameters);

    for (const auto& point : curve.points) {
        require(std::hypot(point.x, point.y) <= 42.0 + 1e-10,
                "points should not exceed the requested radius");
    }
}

void test_rotation()
{
    rosettelab::curves::PolarRoseParameters parameters;
    parameters.samples = 360;
    parameters.rotation_degrees = 90.0;
    const auto curve = rosettelab::curves::generate_polar_rose(parameters);
    require(std::abs(curve.points.front().x) < 1e-10, "rotation should move first point off x axis");
    require(std::abs(curve.points.front().y - 100.0) < 1e-10,
            "rotation should move first point onto y axis");
}

void test_invalid_parameters_are_rejected()
{
    auto parameters = rosettelab::curves::PolarRoseParameters{};
    parameters.radius = 0.0;
    try {
        static_cast<void>(rosettelab::curves::generate_polar_rose(parameters));
        throw std::runtime_error("zero radius should have been rejected");
    } catch (const std::invalid_argument&) {
    }
}

} // namespace

int main()
{
    try {
        test_default_curve_is_closed();
        test_radius_bounds();
        test_rotation();
        test_invalid_parameters_are_rejected();
        std::cout << "All RosetteLab core tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
