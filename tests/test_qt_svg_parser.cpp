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
    layer.appearance.stroke_enabled = false;
    layer.appearance.fill_enabled = true;
    layer.appearance.fill = {0.1, 0.2, 0.3, 0.4};
    layer.appearance.blend_mode = rosettelab::document::BlendMode::Multiply;
    layer.preset_id = "rose-seven";
    layer.preset_customized = true;
    layer.transform = {12.5, -8.0, 1.5, 0.75, false, 30.0};
    layer.copies = {5, 17.0, 0.9, 2.0, -1.0};
    const auto expected_fill = layer.appearance.fill;
    const auto expected_transform = layer.transform;
    const auto expected_copies = layer.copies;
    rosettelab::curves::EllipseParameters ellipse_parameters;
    ellipse_parameters.radius_x = 75.0;
    ellipse_parameters.radius_y = 75.0;
    ellipse_parameters.link_radii = true;
    ellipse_parameters.rotation_degrees = 15.0;
    static_cast<void>(source.add_ellipse(ellipse_parameters, "Tilted ellipse"));
    rosettelab::curves::TrochoidParameters trochoid_parameters;
    trochoid_parameters.fixed_radius = 32.0;
    trochoid_parameters.rolling_radius = 63.0;
    trochoid_parameters.pen_offset = 44.5;
    trochoid_parameters.trace_mode = rosettelab::curves::TraceMode::Limited;
    trochoid_parameters.turns = 2.0;
    static_cast<void>(source.add_trochoid(
        rosettelab::document::CurveType::Epitrochoid,
        trochoid_parameters,
        "Two-turn orbit"));
    rosettelab::curves::LissajousParameters lissajous_parameters;
    lissajous_parameters.frequency_x = 5;
    lissajous_parameters.frequency_y = 4;
    lissajous_parameters.phase_y_degrees = 117.0;
    static_cast<void>(source.add_lissajous(lissajous_parameters, "Paper example"));

    const auto text = rosettelab::svg::serialize_rosettelab_svg(source);
    const auto loaded = rosettelab::svg::parse_rosettelab_svg(QByteArray::fromStdString(text));
    require(loaded.layers().size() == 4, "all implemented curve families should round-trip");
    require(loaded.settings().page_width == 297.0 && loaded.settings().page_height == 210.0,
            "page dimensions should round-trip");
    require(color_close(loaded.settings().background, source.settings().background),
            "page background should round-trip");
    const auto& restored = loaded.layers().front();
    require(restored.name == "Fractional rose", "name should round-trip");
    require(restored.locked, "lock should round-trip");
    require(!restored.appearance.stroke_enabled, "disabled stroke should round-trip");
    require(color_close(restored.appearance.fill, expected_fill),
            "fill RGBA should round-trip within 8-bit SVG precision");
    require(restored.appearance.blend_mode == rosettelab::document::BlendMode::Multiply,
            "blend mode should round-trip");
    require(restored.preset_id == "rose-seven" && restored.preset_customized,
            "preset identity and customized state should round-trip");
    require(restored.transform == expected_transform,
            "layer transform should round-trip");
    require(restored.copies == expected_copies,
            "copy settings should round-trip");
    const auto& restored_parameters = std::get<rosettelab::curves::PolarRoseParameters>(restored.parameters);
    require(restored_parameters.k_mode == rosettelab::curves::PolarKMode::Fraction,
            "fraction mode should round-trip");
    require(restored_parameters.numerator == 2 && restored_parameters.denominator == 3,
            "exact fraction should round-trip");
    const auto& restored_ellipse = loaded.layers()[1];
    require(restored_ellipse.type == rosettelab::document::CurveType::Ellipse,
            "ellipse type should round-trip");
    const auto& ellipse = std::get<rosettelab::curves::EllipseParameters>(restored_ellipse.parameters);
    require(ellipse.radius_x == 75.0 && ellipse.radius_y == 75.0 && ellipse.link_radii &&
            ellipse.rotation_degrees == 15.0,
            "ellipse parameters should round-trip");
    const auto& restored_trochoid = loaded.layers()[2];
    require(restored_trochoid.type == rosettelab::document::CurveType::Epitrochoid,
            "epitrochoid type should round-trip");
    const auto& trochoid = std::get<rosettelab::curves::TrochoidParameters>(
        restored_trochoid.parameters);
    require(trochoid.fixed_radius == 32.0 && trochoid.rolling_radius == 63.0 &&
            trochoid.pen_offset == 44.5 && trochoid.turns == 2.0 &&
            trochoid.trace_mode == rosettelab::curves::TraceMode::Limited,
            "trochoid parameters should round-trip");
    const auto& lissajous = std::get<rosettelab::curves::LissajousParameters>(
        loaded.layers()[3].parameters);
    require(lissajous.frequency_x == 5 && lissajous.frequency_y == 4 &&
            lissajous.phase_y_degrees == 117.0,
            "Lissajous parameters should round-trip");
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
