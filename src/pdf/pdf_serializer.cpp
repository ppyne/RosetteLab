#include "rosettelab/pdf/pdf_serializer.hpp"

#include "rosettelab/curves/ellipse.hpp"
#include "rosettelab/curves/harmonograph.hpp"
#include "rosettelab/curves/lissajous.hpp"
#include "rosettelab/curves/polar_rose.hpp"
#include "rosettelab/curves/trochoid.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace rosettelab::pdf {
namespace {

constexpr double points_per_mm = 72.0 / 25.4;
constexpr double pi = 3.14159265358979323846;

std::string number(const double value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6) << (std::abs(value) < 0.0000005 ? 0.0 : value);
    auto result = stream.str();
    while (result.size() > 1 && result.back() == '0') result.pop_back();
    if (!result.empty() && result.back() == '.') result.pop_back();
    return result;
}

core::BezierPath layer_path(const document::CurveLayer& layer)
{
    if (const auto* p = std::get_if<curves::PolarRoseParameters>(&layer.parameters))
        return curves::generate_polar_rose_bezier(*p, p->bezier_tolerance);
    if (const auto* p = std::get_if<curves::EllipseParameters>(&layer.parameters))
        return curves::generate_ellipse_bezier(*p, p->bezier_tolerance);
    if (const auto* p = std::get_if<curves::TrochoidParameters>(&layer.parameters)) {
        const auto kind = layer.type == document::CurveType::Hypotrochoid
            ? curves::TrochoidKind::Hypotrochoid : curves::TrochoidKind::Epitrochoid;
        return curves::generate_trochoid_bezier(kind, *p, p->bezier_tolerance);
    }
    if (const auto* p = std::get_if<curves::LissajousParameters>(&layer.parameters))
        return curves::generate_lissajous_bezier(*p, p->bezier_tolerance);
    if (const auto* p = std::get_if<curves::HarmonographParameters>(&layer.parameters))
        return curves::generate_harmonograph_bezier(*p, p->bezier_tolerance);
    return {};
}

const char* blend_name(const document::BlendMode mode)
{
    using enum document::BlendMode;
    switch (mode) {
    case Normal: return "Normal";
    case Multiply: return "Multiply";
    case Screen: return "Screen";
    case Overlay: return "Overlay";
    case Darken: return "Darken";
    case Lighten: return "Lighten";
    case ColorDodge: return "ColorDodge";
    case ColorBurn: return "ColorBurn";
    case HardLight: return "HardLight";
    case SoftLight: return "SoftLight";
    case Difference: return "Difference";
    case Exclusion: return "Exclusion";
    case Hue: return "Hue";
    case Saturation: return "Saturation";
    case Color: return "Color";
    case Luminosity: return "Luminosity";
    }
    return "Normal";
}

std::string color_operands(const document::RgbaColor& color, const bool stroke, const ColorModel model)
{
    std::ostringstream out;
    if (model == ColorModel::Rgb) {
        out << number(std::clamp(color.red, 0.0, 1.0)) << ' '
            << number(std::clamp(color.green, 0.0, 1.0)) << ' '
            << number(std::clamp(color.blue, 0.0, 1.0)) << (stroke ? " RG\n" : " rg\n");
    } else {
        const double r = std::clamp(color.red, 0.0, 1.0);
        const double g = std::clamp(color.green, 0.0, 1.0);
        const double b = std::clamp(color.blue, 0.0, 1.0);
        const double k = 1.0 - std::max({r, g, b});
        const double denominator = 1.0 - k;
        const double c = denominator > 0.0 ? (1.0 - r - k) / denominator : 0.0;
        const double m = denominator > 0.0 ? (1.0 - g - k) / denominator : 0.0;
        const double y = denominator > 0.0 ? (1.0 - b - k) / denominator : 0.0;
        out << number(c) << ' ' << number(m) << ' ' << number(y) << ' ' << number(k)
            << (stroke ? " K\n" : " k\n");
    }
    return out.str();
}

std::string path_commands(const core::BezierPath& path)
{
    if (path.segments.empty()) return {};
    std::ostringstream out;
    out << number(path.segments.front().start.x) << ' '
        << number(path.segments.front().start.y) << " m\n";
    for (const auto& segment : path.segments) {
        out << number(segment.control1.x) << ' ' << number(segment.control1.y) << ' '
            << number(segment.control2.x) << ' ' << number(segment.control2.y) << ' '
            << number(segment.end.x) << ' ' << number(segment.end.y) << " c\n";
    }
    if (path.closed) out << "h\n";
    return out.str();
}

std::string paint_operator(const document::LayerAppearance& appearance)
{
    if (appearance.fill_enabled && appearance.stroke_enabled)
        return appearance.fill_rule == document::FillRule::EvenOdd ? "B*\n" : "B\n";
    if (appearance.fill_enabled)
        return appearance.fill_rule == document::FillRule::EvenOdd ? "f*\n" : "f\n";
    if (appearance.stroke_enabled) return "S\n";
    return "n\n";
}

struct PdfObjects {
    std::vector<std::string> values;
    int reserve() { values.emplace_back(); return static_cast<int>(values.size()); }
    int add(std::string value) { values.push_back(std::move(value)); return static_cast<int>(values.size()); }
    void set(const int id, std::string value) { values.at(static_cast<std::size_t>(id - 1)) = std::move(value); }
};

std::string stream_object(const std::string& dictionary, const std::string& content)
{
    return "<< " + dictionary + " /Length " + std::to_string(content.size()) +
        " >>\nstream\n" + content + "endstream";
}

} // namespace

std::string serialize_vector_pdf(const document::Document& document, const ExportOptions& options)
{
    PdfObjects objects;
    const int catalog_id = objects.reserve();
    const int pages_id = objects.reserve();
    const int page_id = objects.reserve();
    const int page_content_id = objects.reserve();

    struct LayerResources { int form{}; int group_state{}; };
    std::vector<LayerResources> layers;
    for (const auto& layer : document.layers()) {
        if (!layer.visible) continue;
        core::BezierPath curve;
        try { curve = layer_path(layer); } catch (...) { continue; }
        if (curve.segments.empty()) continue;

        const auto& appearance = layer.appearance;
        const int path_state_id = objects.add(
            "<< /Type /ExtGState /CA " + number(appearance.stroke.alpha) +
            " /ca " + number(appearance.fill.alpha) + " >>");
        const int group_state_id = objects.add(
            "<< /Type /ExtGState /BM /" + std::string(blend_name(appearance.blend_mode)) +
            " /CA " + number(std::clamp(appearance.opacity, 0.0, 1.0)) +
            " /ca " + number(std::clamp(appearance.opacity, 0.0, 1.0)) + " >>");

        std::ostringstream content;
        content << "q\n/PathGS gs\n";
        if (appearance.stroke_enabled) {
            content << color_operands(appearance.stroke, true, options.color_model)
                << number(std::max(0.0, appearance.stroke_width)) << " w\n";
        }
        if (appearance.fill_enabled)
            content << color_operands(appearance.fill, false, options.color_model);
        const auto commands = path_commands(curve);
        const auto paint = paint_operator(appearance);
        const int count = std::clamp(layer.copies.count, 1, 1000);
        for (int copy = 0; copy < count; ++copy) {
            const auto placement = document::copy_placement(layer, copy);
            const double angle = placement.rotation_degrees * pi / 180.0;
            const double sx = layer.transform.scale_x * placement.scale;
            const double sy = layer.transform.scale_y * placement.scale;
            const double c = std::cos(angle);
            const double s = std::sin(angle);
            content << "q\n" << number(c * sx) << ' ' << number(s * sx) << ' '
                << number(-s * sy) << ' ' << number(c * sy) << ' '
                << number(placement.position_x) << ' ' << number(placement.position_y) << " cm\n"
                << commands << paint << "Q\n";
        }
        content << "Q\n";

        const double half_width = document.settings().page_width / 2.0;
        const double half_height = document.settings().page_height / 2.0;
        const std::string dictionary =
            "/Type /XObject /Subtype /Form /FormType 1 /BBox [" + number(-half_width) + " " +
            number(-half_height) + " " + number(half_width) + " " + number(half_height) +
            "] /Group << /S /Transparency /I true /K false >> /Resources << /ExtGState << /PathGS " +
            std::to_string(path_state_id) + " 0 R >> >>";
        const int form_id = objects.add(stream_object(dictionary, content.str()));
        layers.push_back({form_id, group_state_id});
    }

    const double width = document.settings().page_width * points_per_mm;
    const double height = document.settings().page_height * points_per_mm;
    std::ostringstream resources;
    resources << "<< /XObject << ";
    for (std::size_t i = 0; i < layers.size(); ++i)
        resources << "/Layer" << i << ' ' << layers[i].form << " 0 R ";
    resources << ">> /ExtGState << ";
    for (std::size_t i = 0; i < layers.size(); ++i)
        resources << "/LayerGS" << i << ' ' << layers[i].group_state << " 0 R ";
    resources << ">> >>";

    std::ostringstream page_content;
    const auto& background = document.settings().background;
    if (background.alpha > 0.0) {
        const int background_state = objects.add(
            "<< /Type /ExtGState /ca " + number(background.alpha) + " >>");
        // Add the background state after the initially constructed resources.
        auto resource_text = resources.str();
        const auto marker = resource_text.rfind(">> >>");
        resource_text.insert(marker, "/BackgroundGS " + std::to_string(background_state) + " 0 R ");
        resources.str({}); resources.clear(); resources << resource_text;
        page_content << "q\n/BackgroundGS gs\n"
            << color_operands(background, false, options.color_model)
            << "0 0 " << number(width) << ' ' << number(height) << " re f\nQ\n";
    }
    page_content << "q\n" << number(points_per_mm) << " 0 0 " << number(-points_per_mm) << ' '
        << number(width / 2.0) << ' ' << number(height / 2.0) << " cm\n";
    for (std::size_t i = 0; i < layers.size(); ++i)
        page_content << "q\n/LayerGS" << i << " gs\n/Layer" << i << " Do\nQ\n";
    page_content << "Q\n";

    objects.set(page_content_id, stream_object({}, page_content.str()));
    objects.set(page_id,
        "<< /Type /Page /Parent " + std::to_string(pages_id) + " 0 R /MediaBox [0 0 " +
        number(width) + " " + number(height) + "] /Group << /S /Transparency /CS /" +
        std::string(options.color_model == ColorModel::Rgb ? "DeviceRGB" : "DeviceCMYK") +
        " >> /Resources " + resources.str() + " /Contents " + std::to_string(page_content_id) + " 0 R >>");
    objects.set(pages_id, "<< /Type /Pages /Kids [" + std::to_string(page_id) + " 0 R] /Count 1 >>");
    objects.set(catalog_id, "<< /Type /Catalog /Pages " + std::to_string(pages_id) + " 0 R >>");

    std::string pdf = "%PDF-1.7\n%\xE2\xE3\xCF\xD3\n";
    std::vector<std::size_t> offsets{0};
    for (std::size_t i = 0; i < objects.values.size(); ++i) {
        offsets.push_back(pdf.size());
        pdf += std::to_string(i + 1) + " 0 obj\n" + objects.values[i] + "\nendobj\n";
    }
    const auto xref = pdf.size();
    pdf += "xref\n0 " + std::to_string(objects.values.size() + 1) + "\n";
    pdf += "0000000000 65535 f \n";
    for (std::size_t i = 1; i < offsets.size(); ++i) {
        std::ostringstream entry;
        entry << std::setw(10) << std::setfill('0') << offsets[i] << " 00000 n \n";
        pdf += entry.str();
    }
    pdf += "trailer\n<< /Size " + std::to_string(objects.values.size() + 1) +
        " /Root " + std::to_string(catalog_id) + " 0 R >>\nstartxref\n" +
        std::to_string(xref) + "\n%%EOF\n";
    return pdf;
}

} // namespace rosettelab::pdf
