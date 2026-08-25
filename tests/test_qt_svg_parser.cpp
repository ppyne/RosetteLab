#include "svg/qt_svg_parser.hpp"

#include "rosettelab/svg/svg_serializer.hpp"

#include <QByteArray>

#include <exception>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void require(const bool condition, const std::string_view message)
{
    if (!condition) throw std::runtime_error(std::string(message));
}

bool color_close(
    const rosettelab::document::RgbaColor& left,
    const rosettelab::document::RgbaColor& right)
{
    constexpr double tolerance = 1.0 / 255.0 + 1e-12;
    return std::abs(left.red - right.red) <= tolerance &&
           std::abs(left.green - right.green) <= tolerance &&
           std::abs(left.blue - right.blue) <= tolerance &&
           std::abs(left.alpha - right.alpha) <= tolerance;
}

void test_save_open_round_trip()
{
    rosettelab::document::Document source;
    source.settings().page_width = 297.0;
    source.settings().page_height = 210.0;
    source.settings().background = {0.2, 0.3, 0.4, 0.5};
    auto parameters = rosettelab::curves::PolarRoseParameters{};
    parameters.k_mode = rosettelab::curves::PolarKMode::Fraction;
    parameters.numerator = 2;
    parameters.denominator = 3;
    auto& layer = source.add_polar_rose(parameters, "Fractional rose");
    layer.locked = true;
    layer.appearance.fill_enabled = true;
    layer.appearance.fill = {0.1, 0.2, 0.3, 0.4};
    layer.appearance.blend_mode = rosettelab::document::BlendMode::Multiply;

    const auto text = rosettelab::svg::serialize_rosettelab_svg(source);
    const auto loaded = rosettelab::svg::parse_rosettelab_svg(QByteArray::fromStdString(text));
    require(loaded.layers().size() == 1, "one layer should round-trip");
    require(loaded.settings().page_width == 297.0 && loaded.settings().page_height == 210.0,
            "page dimensions should round-trip");
    require(color_close(loaded.settings().background, source.settings().background),
            "page background should round-trip");
    const auto& restored = loaded.layers().front();
    require(restored.name == "Fractional rose", "name should round-trip");
    require(restored.locked, "lock should round-trip");
    require(color_close(restored.appearance.fill, layer.appearance.fill),
            "fill RGBA should round-trip within 8-bit SVG precision");
    require(restored.appearance.blend_mode == rosettelab::document::BlendMode::Multiply,
            "blend mode should round-trip");
    const auto& restored_parameters = std::get<rosettelab::curves::PolarRoseParameters>(restored.parameters);
    require(restored_parameters.k_mode == rosettelab::curves::PolarKMode::Fraction,
            "fraction mode should round-trip");
    require(restored_parameters.numerator == 2 && restored_parameters.denominator == 3,
            "exact fraction should round-trip");
}

void test_rejects_ordinary_or_unsafe_svg()
{
    for (const auto* text : {
             "<svg xmlns=\"http://www.w3.org/2000/svg\"/>",
             "<!DOCTYPE svg><svg xmlns=\"http://www.w3.org/2000/svg\"/>",
         }) {
        bool rejected = false;
        try {
            static_cast<void>(rosettelab::svg::parse_rosettelab_svg(text));
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected, "ordinary and unsafe SVG must be rejected");
    }
}

} // namespace

int main()
{
    try {
        test_save_open_round_trip();
        test_rejects_ordinary_or_unsafe_svg();
        std::cout << "All RosetteLab Qt SVG parser tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
