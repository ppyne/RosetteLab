#include "rosettelab/svg/svg_serializer.hpp"

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

bool contains(const std::string& text, const std::string_view fragment)
{
    return text.find(fragment) != std::string::npos;
}

void test_native_svg_contains_geometry_and_metadata()
{
    rosettelab::document::Document document;
    document.settings().page_width = 297.0;
    document.settings().page_height = 210.0;
    document.settings().background = {0.25, 0.5, 0.75, 0.5};
    auto& layer = document.add_polar_rose({}, "Rose & <one>");
    auto& parameters = std::get<rosettelab::curves::PolarRoseParameters>(layer.parameters);
    parameters.k_mode = rosettelab::curves::PolarKMode::Fraction;
    parameters.numerator = 1;
    parameters.denominator = 3;
    layer.appearance.stroke = {1.0, 0.0, 0.5, 0.25};
    layer.appearance.fill_enabled = true;
    layer.appearance.fill = {0.0, 0.5, 1.0, 0.75};
    layer.appearance.fill_rule = rosettelab::document::FillRule::EvenOdd;
    layer.appearance.opacity = 0.8;
    layer.appearance.blend_mode = rosettelab::document::BlendMode::Multiply;

    const auto svg = rosettelab::svg::serialize_rosettelab_svg(document);
    require(contains(svg, "xmlns:rosettelab=\"https://rosettelab.app/ns/1\""),
            "native namespace should be declared");
    require(contains(svg, "rosettelab:document=\"true\""), "document marker should be present");
    require(contains(svg, "rosettelab:page-width=\"297\""), "page width should be stored");
    require(contains(svg, "rosettelab:page-height=\"210\""), "page height should be stored");
    require(contains(svg, "rosettelab:background=\"#4080BF\""), "page background should be stored");
    require(contains(svg, "rosettelab:name=\"Rose &amp; &lt;one&gt;\""), "layer name should be escaped");
    require(contains(svg, "rosettelab:type=\"polar-rose\""), "curve type should be stored");
    require(contains(svg, "bezier-tolerance=\"0.05\""), "curve tolerance should be stored");
    require(contains(svg, "k-mode=\"fraction\""), "fraction mode should be stored");
    require(contains(svg, "numerator=\"1\""), "fraction numerator should be stored");
    require(contains(svg, "denominator=\"3\""), "fraction denominator should be stored");
    require(contains(svg, "<path d=\"M "), "rendered path should be present");
    require(contains(svg, " C "), "rendered path should use cubic Bezier commands");
    require(contains(svg, "stroke=\"#FF0080\""), "stroke RGB should be serialized");
    require(contains(svg, "stroke-opacity=\"0.25\""), "stroke alpha should be serialized");
    require(contains(svg, "fill-rule=\"evenodd\""), "fill rule should be serialized");
    require(contains(svg, "mix-blend-mode:multiply"), "blend mode should be serialized");
}

void test_hidden_layer_remains_in_project()
{
    rosettelab::document::Document document;
    const auto id = document.add_polar_rose().id;
    static_cast<void>(document.set_layer_visible(id, false));
    static_cast<void>(document.set_layer_locked(id, true));
    const auto svg = rosettelab::svg::serialize_rosettelab_svg(document);
    require(contains(svg, "rosettelab:visible=\"false\""), "hidden state should be stored");
    require(contains(svg, "rosettelab:locked=\"true\""), "lock state should be stored");
    require(contains(svg, "display=\"none\""), "hidden layer should not render in standard viewers");
    require(contains(svg, "<path d=\"M "), "hidden editable geometry should remain in the file");
}

void test_ellipse_contains_editable_metadata_and_beziers()
{
    rosettelab::document::Document document;
    rosettelab::curves::EllipseParameters parameters;
    parameters.radius_x = 90.0;
    parameters.radius_y = 90.0;
    parameters.link_radii = true;
    parameters.rotation_degrees = 27.0;
    static_cast<void>(document.add_ellipse(parameters));
    const auto svg = rosettelab::svg::serialize_rosettelab_svg(document);
    require(contains(svg, "rosettelab:type=\"ellipse\""), "ellipse type should be stored");
    require(contains(svg, "radius-x=\"90\""), "horizontal radius should be stored");
    require(contains(svg, "radius-y=\"90\""), "vertical radius should be stored");
    require(contains(svg, "link-radii=\"true\""), "linked radii should be stored");
    require(contains(svg, "rotation-degrees=\"27\""), "ellipse rotation should be stored");
    require(contains(svg, " C "), "ellipse should render as cubic Bezier segments");
}

void test_trochoid_contains_trace_metadata()
{
    rosettelab::document::Document document;
    rosettelab::curves::TrochoidParameters parameters;
    parameters.fixed_radius = 32.0;
    parameters.rolling_radius = 63.0;
    parameters.pen_offset = 44.5;
    parameters.trace_mode = rosettelab::curves::TraceMode::Limited;
    parameters.turns = 2.0;
    static_cast<void>(document.add_trochoid(
        rosettelab::document::CurveType::Epitrochoid, parameters));
    const auto svg = rosettelab::svg::serialize_rosettelab_svg(document);
    require(contains(svg, "rosettelab:type=\"epitrochoid\""), "trochoid type should be stored");
    require(contains(svg, "fixed-radius=\"32\""), "fixed radius should be stored");
    require(contains(svg, "rolling-radius=\"63\""), "rolling radius should be stored");
    require(contains(svg, "pen-offset=\"44.5\""), "pen offset should be stored");
    require(contains(svg, "trace-mode=\"limited\""), "trace mode should be stored");
    require(contains(svg, "turns=\"2\""), "limited turn count should be stored");
}

} // namespace

int main()
{
    try {
        test_native_svg_contains_geometry_and_metadata();
        test_hidden_layer_remains_in_project();
        test_ellipse_contains_editable_metadata_and_beziers();
        test_trochoid_contains_trace_metadata();
        std::cout << "All RosetteLab SVG serializer tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
