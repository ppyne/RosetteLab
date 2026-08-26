#include "rosettelab/svg/svg_serializer.hpp"

#include "rosettelab/curves/ellipse.hpp"
#include "rosettelab/curves/polar_rose.hpp"
#include "rosettelab/curves/trochoid.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace rosettelab::svg {
namespace {

std::string number(const double value)
{
    if (!std::isfinite(value)) {
        throw std::invalid_argument("SVG values must be finite");
    }
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(12) << value;
    return stream.str();
}

std::string xml_escape(const std::string_view text)
{
    std::string escaped;
    escaped.reserve(text.size());
    for (const char character : text) {
        switch (character) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped += character; break;
        }
    }
    return escaped;
}

int color_component(const double value)
{
    return static_cast<int>(std::lround(std::clamp(value, 0.0, 1.0) * 255.0));
}

std::string rgb_hex(const document::RgbaColor& color)
{
    std::ostringstream stream;
    stream << '#' << std::uppercase << std::hex << std::setfill('0')
           << std::setw(2) << color_component(color.red)
           << std::setw(2) << color_component(color.green)
           << std::setw(2) << color_component(color.blue);
    return stream.str();
}

const char* fill_rule_name(const document::FillRule rule)
{
    return rule == document::FillRule::EvenOdd ? "evenodd" : "nonzero";
}

const char* blend_mode_name(const document::BlendMode mode)
{
    using enum document::BlendMode;
    switch (mode) {
    case Normal: return "normal";
    case Multiply: return "multiply";
    case Screen: return "screen";
    case Overlay: return "overlay";
    case Darken: return "darken";
    case Lighten: return "lighten";
    case ColorDodge: return "color-dodge";
    case ColorBurn: return "color-burn";
    case HardLight: return "hard-light";
    case SoftLight: return "soft-light";
    case Difference: return "difference";
    case Exclusion: return "exclusion";
    case Hue: return "hue";
    case Saturation: return "saturation";
    case Color: return "color";
    case Luminosity: return "luminosity";
    }
    return "normal";
}

std::string path_data(const core::BezierPath& path)
{
    if (path.segments.empty()) {
        return {};
    }
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::setprecision(12);
    const auto& start = path.segments.front().start;
    output << "M " << start.x << ' ' << start.y;
    for (const auto& segment : path.segments) {
        output << " C "
               << segment.control1.x << ' ' << segment.control1.y << ' '
               << segment.control2.x << ' ' << segment.control2.y << ' '
               << segment.end.x << ' ' << segment.end.y;
    }
    if (path.closed) {
        output << " Z";
    }
    return output.str();
}

void write_rendered_path(
    std::ostringstream& output,
    const core::BezierPath& path,
    const document::LayerAppearance& appearance)
{
    output << "    <path d=\"" << path_data(path) << "\""
           << " stroke=\"" << (appearance.stroke_enabled ? rgb_hex(appearance.stroke) : "none") << "\""
           << " rosettelab:stroke-color=\"" << rgb_hex(appearance.stroke) << "\""
           << " stroke-opacity=\"" << number(std::clamp(appearance.stroke.alpha, 0.0, 1.0)) << "\""
           << " stroke-width=\"" << number(std::max(0.0, appearance.stroke_width)) << "\""
           << " fill=\"" << (appearance.fill_enabled ? rgb_hex(appearance.fill) : "none") << "\"";
    if (appearance.fill_enabled) {
        output << " fill-opacity=\"" << number(std::clamp(appearance.fill.alpha, 0.0, 1.0)) << "\"";
    }
    output << " fill-rule=\"" << fill_rule_name(appearance.fill_rule) << "\""
           << " opacity=\"" << number(std::clamp(appearance.opacity, 0.0, 1.0)) << "\""
           << " style=\"mix-blend-mode:" << blend_mode_name(appearance.blend_mode) << "\"/>\n";
}

void write_polar_rose(std::ostringstream& output, const document::CurveLayer& layer)
{
    const auto* parameters = std::get_if<curves::PolarRoseParameters>(&layer.parameters);
    if (parameters == nullptr) {
        throw std::invalid_argument("Polar rose layer has incompatible parameters");
    }
    const auto path = curves::generate_polar_rose_bezier(
        *parameters, parameters->bezier_tolerance);

    output << "    <rosettelab:curve"
           << " radius=\"" << number(parameters->radius) << "\""
           << " k-mode=\"" << (parameters->k_mode == curves::PolarKMode::Fraction ? "fraction" : "decimal") << "\""
           << " k=\"" << number(parameters->k) << "\""
           << " numerator=\"" << parameters->numerator << "\""
           << " denominator=\"" << parameters->denominator << "\""
           << " effective-k=\"" << number(curves::effective_k(*parameters)) << "\""
           << " phase-degrees=\"" << number(parameters->phase_degrees) << "\""
           << " rotation-degrees=\"" << number(parameters->rotation_degrees) << "\""
           << " bezier-tolerance=\"" << number(parameters->bezier_tolerance) << "\"/>\n";

    write_rendered_path(output, path, layer.appearance);
}

void write_ellipse(std::ostringstream& output, const document::CurveLayer& layer)
{
    const auto* parameters = std::get_if<curves::EllipseParameters>(&layer.parameters);
    if (parameters == nullptr) {
        throw std::invalid_argument("Ellipse layer has incompatible parameters");
    }
    const auto path = curves::generate_ellipse_bezier(
        *parameters, parameters->bezier_tolerance);
    output << "    <rosettelab:curve"
           << " radius-x=\"" << number(parameters->radius_x) << "\""
           << " radius-y=\"" << number(parameters->radius_y) << "\""
           << " link-radii=\"" << (parameters->link_radii ? "true" : "false") << "\""
           << " rotation-degrees=\"" << number(parameters->rotation_degrees) << "\""
           << " bezier-tolerance=\"" << number(parameters->bezier_tolerance) << "\"/>\n";
    write_rendered_path(output, path, layer.appearance);
}

void write_trochoid(std::ostringstream& output, const document::CurveLayer& layer)
{
    const auto* parameters = std::get_if<curves::TrochoidParameters>(&layer.parameters);
    if (parameters == nullptr) {
        throw std::invalid_argument("Trochoid layer has incompatible parameters");
    }
    const auto kind = layer.type == document::CurveType::Hypotrochoid
        ? curves::TrochoidKind::Hypotrochoid
        : curves::TrochoidKind::Epitrochoid;
    const auto path = curves::generate_trochoid_bezier(
        kind, *parameters, parameters->bezier_tolerance);
    output << "    <rosettelab:curve"
           << " fixed-radius=\"" << number(parameters->fixed_radius) << "\""
           << " rolling-radius=\"" << number(parameters->rolling_radius) << "\""
           << " pen-offset=\"" << number(parameters->pen_offset) << "\""
           << " rotation-degrees=\"" << number(parameters->rotation_degrees) << "\""
           << " trace-mode=\"" << (parameters->trace_mode == curves::TraceMode::Complete ? "complete" : "limited") << "\""
           << " turns=\"" << number(parameters->turns) << "\""
           << " close-limited-path=\"" << (parameters->close_limited_path ? "true" : "false") << "\""
           << " bezier-tolerance=\"" << number(parameters->bezier_tolerance) << "\"/>\n";
    write_rendered_path(output, path, layer.appearance);
}

const char* curve_type_id(const document::CurveType type)
{
    switch (type) {
    case document::CurveType::PolarRose: return "polar-rose";
    case document::CurveType::Ellipse: return "ellipse";
    case document::CurveType::Hypotrochoid: return "hypotrochoid";
    case document::CurveType::Epitrochoid: return "epitrochoid";
    default: throw std::invalid_argument("Unsupported curve type for SVG export");
    }
}

} // namespace

std::string serialize_rosettelab_svg(
    const document::Document& document)
{
    const auto& settings = document.settings();
    if (!std::isfinite(settings.page_width) || !std::isfinite(settings.page_height) ||
        settings.page_width <= 0.0 || settings.page_height <= 0.0 || settings.unit.empty()) {
        throw std::invalid_argument("SVG dimensions must be finite and positive");
    }

    const double left = -settings.page_width / 2.0;
    const double top = -settings.page_height / 2.0;
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<svg xmlns=\"http://www.w3.org/2000/svg\""
           << " xmlns:rosettelab=\"" << metadata_namespace << "\""
           << " width=\"" << number(settings.page_width) << xml_escape(settings.unit) << "\""
           << " height=\"" << number(settings.page_height) << xml_escape(settings.unit) << "\""
           << " viewBox=\"" << number(left) << ' ' << number(top) << ' '
           << number(settings.page_width) << ' ' << number(settings.page_height) << "\""
           << " rosettelab:document=\"true\""
           << " rosettelab:schema-version=\"" << schema_version << "\""
           << " rosettelab:page-width=\"" << number(settings.page_width) << "\""
           << " rosettelab:page-height=\"" << number(settings.page_height) << "\""
           << " rosettelab:unit=\"" << xml_escape(settings.unit) << "\""
           << " rosettelab:background=\"" << rgb_hex(settings.background) << "\""
           << " rosettelab:background-opacity=\""
           << number(std::clamp(settings.background.alpha, 0.0, 1.0)) << "\">\n";

    output << "  <rect x=\"" << number(left) << "\" y=\"" << number(top)
           << "\" width=\"" << number(settings.page_width) << "\" height=\"" << number(settings.page_height)
           << "\" fill=\"" << rgb_hex(settings.background) << "\" fill-opacity=\""
           << number(std::clamp(settings.background.alpha, 0.0, 1.0)) << "\""
           << " rosettelab:role=\"page-background\"/>\n";

    for (const auto& layer : document.layers()) {
        output << "  <g id=\"layer-" << layer.id << "\""
               << " rosettelab:id=\"" << layer.id << "\""
               << " rosettelab:name=\"" << xml_escape(layer.name) << "\""
               << " rosettelab:type=\"" << curve_type_id(layer.type) << "\""
               << " rosettelab:visible=\"" << (layer.visible ? "true" : "false") << "\""
               << " rosettelab:locked=\"" << (layer.locked ? "true" : "false") << "\"";
        if (!layer.visible) {
            output << " display=\"none\"";
        }
        output << ">\n";
        if (layer.type == document::CurveType::PolarRose) {
            write_polar_rose(output, layer);
        } else if (layer.type == document::CurveType::Ellipse) {
            write_ellipse(output, layer);
        } else if (layer.type == document::CurveType::Hypotrochoid ||
                   layer.type == document::CurveType::Epitrochoid) {
            write_trochoid(output, layer);
        }
        output << "  </g>\n";
    }
    output << "</svg>\n";
    return output.str();
}

} // namespace rosettelab::svg
