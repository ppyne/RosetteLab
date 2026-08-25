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
    auto& layer = document.add_polar_rose({}, "Rose & <one>");
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
    require(contains(svg, "rosettelab:name=\"Rose &amp; &lt;one&gt;\""), "layer name should be escaped");
    require(contains(svg, "rosettelab:type=\"polar-rose\""), "curve type should be stored");
    require(contains(svg, "bezier-tolerance=\"0.05\""), "curve tolerance should be stored");
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

} // namespace

int main()
{
    try {
        test_native_svg_contains_geometry_and_metadata();
        test_hidden_layer_remains_in_project();
        std::cout << "All RosetteLab SVG serializer tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
