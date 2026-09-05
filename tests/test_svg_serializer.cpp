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

std::size_t occurrence_count(const std::string& text, const std::string_view fragment)
{
    std::size_t count = 0;
    std::size_t position = 0;
    while ((position = text.find(fragment, position)) != std::string::npos) {
        ++count;
        position += fragment.size();
    }
    return count;
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
    layer.transform = {12.5, -8.0, 1.5, 0.75, false, 30.0};
    layer.copies.arrangement = rosettelab::document::CopyArrangement::Linear;
    layer.copies.count = 3;
    layer.copies.rotation_step_degrees = 17.0;
    layer.copies.scale_step = 0.9;
    layer.copies.offset_x_step = 2.0;
    layer.copies.offset_y_step = -1.0;

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
    require(contains(svg, "rosettelab:position-x=\"12.5\""), "layer X position should be stored");
    require(contains(svg, "rosettelab:scale-y=\"0.75\""), "independent Y scale should be stored");
    require(contains(svg, "rosettelab:copy-count=\"3\""), "copy count should be stored");
    require(contains(svg, "rosettelab:copy-arrangement=\"linear\""),
            "copy arrangement should be stored");
    require(contains(svg, "translate(14.5 -9) rotate(47) scale(1.35 0.675)"),
            "second rendered copy should use progressive transform settings");
    require(occurrence_count(svg, "    <path d=\"") == 3,
            "one rendered SVG path should be emitted per copy");
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

void test_disabled_stroke_preserves_editable_color()
{
    rosettelab::document::Document document;
    auto& layer = document.add_polar_rose();
    layer.appearance.stroke_enabled = false;
    layer.appearance.stroke = {0.2, 0.4, 0.6, 0.5};
    const auto svg = rosettelab::svg::serialize_rosettelab_svg(document);
    require(contains(svg, "stroke=\"none\""), "disabled stroke should not render");
    require(contains(svg, "rosettelab:stroke-color=\"#336699\""),
            "disabled stroke color should remain editable");
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

void test_lissajous_contains_editable_metadata()
{
    rosettelab::document::Document document;
    rosettelab::curves::LissajousParameters parameters;
    parameters.frequency_x = 5;
    parameters.frequency_y = 4;
    parameters.phase_y_degrees = 117.0;
    static_cast<void>(document.add_lissajous(parameters));
    const auto svg = rosettelab::svg::serialize_rosettelab_svg(document);
    require(contains(svg, "rosettelab:type=\"lissajous\""), "Lissajous type should be stored");
    require(contains(svg, "frequency-x=\"5\""), "Lissajous X frequency should be stored");
    require(contains(svg, "phase-y-degrees=\"117\""), "Lissajous Y phase should be stored");
}

void test_droplet_rosette_contains_compound_geometry_and_metadata()
{
    rosettelab::document::Document document;
    rosettelab::curves::DropletRosetteParameters parameters;
    parameters.droplets = 5;
    parameters.core_radius = 14.0;
    parameters.swirl_degrees = -32.0;
    static_cast<void>(document.add_droplet_rosette(parameters));
    const auto svg = rosettelab::svg::serialize_rosettelab_svg(document);
    require(contains(svg, "rosettelab:type=\"droplet-rosette\""), "Droplet Rosette type should be stored");
    require(contains(svg, "droplets=\"5\""), "droplet count should be stored");
    require(contains(svg, "core-radius=\"14\""), "core radius should be stored");
    require(contains(svg, "swirl-degrees=\"-32\""), "swirl should be stored");
    require(occurrence_count(svg, " Z M ") == 4,
            "five droplets should serialize as five closed SVG subpaths");
}

void test_preset_state_is_metadata()
{
    rosettelab::document::Document document;
    auto& layer=document.add_polar_rose();
    layer.preset_id="rose-eleven";
    layer.preset_customized=true;
    const auto svg=rosettelab::svg::serialize_rosettelab_svg(document);
    require(contains(svg,"rosettelab:preset-id=\"rose-eleven\""),"preset ID should be stored");
    require(contains(svg,"rosettelab:preset-customized=\"true\""),"customized state should be stored");
}

void test_clean_svg_contains_only_visible_rendered_content()
{
    rosettelab::document::Document document;
    document.settings().background = {0.2, 0.4, 0.6, 0.5};
    auto& visible = document.add_polar_rose({}, "Visible & clean");
    visible.copies.count = 3;
    visible.copies.rotation_step_degrees = 12.0;
    visible.appearance.fill_enabled = true;
    visible.appearance.blend_mode = rosettelab::document::BlendMode::Multiply;
    auto& hidden = document.add_ellipse({}, "Hidden layer");
    hidden.visible = false;

    const auto svg = rosettelab::svg::serialize_clean_svg(document);
    require(!contains(svg, "xmlns:rosettelab"),
            "clean SVG should not declare the RosetteLab namespace");
    require(!contains(svg, "rosettelab:"),
            "clean SVG should not contain RosetteLab metadata attributes or elements");
    require(contains(svg, "<title>Visible &amp; clean</title>"),
            "clean SVG should retain the visible layer name as a standard title");
    require(!contains(svg, "Hidden layer"),
            "clean SVG should omit hidden layers");
    require(occurrence_count(svg, "    <path d=\"") == 3,
            "clean SVG should render every parametric copy");
    require(contains(svg, "fill-opacity=\"0.5\""),
            "clean SVG should retain document background transparency");
    require(contains(svg, "mix-blend-mode:multiply"),
            "clean SVG should retain visible blend modes");
}

} // namespace

int main()
{
    try {
        test_native_svg_contains_geometry_and_metadata();
        test_hidden_layer_remains_in_project();
        test_disabled_stroke_preserves_editable_color();
        test_ellipse_contains_editable_metadata_and_beziers();
        test_trochoid_contains_trace_metadata();
        test_lissajous_contains_editable_metadata();
        test_droplet_rosette_contains_compound_geometry_and_metadata();
        test_preset_state_is_metadata();
        test_clean_svg_contains_only_visible_rendered_content();
        std::cout << "All RosetteLab SVG serializer tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
