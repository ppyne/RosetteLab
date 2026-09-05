#include "svg/qt_svg_parser.hpp"

#include "rosettelab/svg/svg_serializer.hpp"

#include <QXmlStreamReader>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace rosettelab::svg {
namespace {

std::runtime_error parse_error(const QString& message)
{
    return std::runtime_error(message.toStdString());
}

QString required_attribute(
    const QXmlStreamAttributes& attributes,
    const QString& name)
{
    const auto value = attributes.value(name);
    if (value.isNull()) {
        throw parse_error(QStringLiteral("Missing SVG attribute: %1").arg(name));
    }
    return value.toString();
}

QString required_metadata_attribute(
    const QXmlStreamAttributes& attributes,
    const QString& metadata_ns,
    const QString& name)
{
    const auto value = attributes.value(metadata_ns, name);
    if (value.isNull()) {
        throw parse_error(QStringLiteral("Missing RosetteLab attribute: %1").arg(name));
    }
    return value.toString();
}

double parse_double(const QString& text, const QString& field)
{
    bool ok = false;
    const double value = text.toDouble(&ok);
    if (!ok || !std::isfinite(value)) {
        throw parse_error(QStringLiteral("Invalid numeric value for %1").arg(field));
    }
    return value;
}

int parse_integer(const QString& text, const QString& field)
{
    bool ok = false;
    const int value = text.toInt(&ok);
    if (!ok) {
        throw parse_error(QStringLiteral("Invalid integer value for %1").arg(field));
    }
    return value;
}

bool parse_boolean(const QString& text, const QString& field)
{
    if (text == "true") return true;
    if (text == "false") return false;
    throw parse_error(QStringLiteral("Invalid boolean value for %1").arg(field));
}

double optional_metadata_double(
    const QXmlStreamAttributes& attributes, const QString& metadata_ns,
    const QString& name, const double fallback)
{
    const auto value = attributes.value(metadata_ns, name);
    return value.isNull() ? fallback : parse_double(value.toString(), name);
}

int optional_metadata_integer(
    const QXmlStreamAttributes& attributes, const QString& metadata_ns,
    const QString& name, const int fallback)
{
    const auto value = attributes.value(metadata_ns, name);
    return value.isNull() ? fallback : parse_integer(value.toString(), name);
}

bool optional_metadata_boolean(
    const QXmlStreamAttributes& attributes, const QString& metadata_ns,
    const QString& name, const bool fallback)
{
    const auto value = attributes.value(metadata_ns, name);
    return value.isNull() ? fallback : parse_boolean(value.toString(), name);
}

document::RgbaColor parse_rgb(const QString& text, const double alpha)
{
    if (text.size() != 7 || !text.startsWith('#')) {
        throw parse_error("RosetteLab colors must use #RRGGBB");
    }
    bool ok = false;
    const auto packed = text.mid(1).toUInt(&ok, 16);
    if (!ok) {
        throw parse_error("Invalid SVG color");
    }
    return {
        static_cast<double>((packed >> 16) & 0xff) / 255.0,
        static_cast<double>((packed >> 8) & 0xff) / 255.0,
        static_cast<double>(packed & 0xff) / 255.0,
        std::clamp(alpha, 0.0, 1.0),
    };
}

document::RgbaColor parse_rgba(const QString& text)
{
    if (text.size() != 9 || !text.startsWith('#')) {
        throw parse_error("Palette colors must use #RRGGBBAA");
    }
    bool ok = false;
    const auto packed = text.mid(1).toULongLong(&ok, 16);
    if (!ok) throw parse_error("Invalid palette color");
    return {
        static_cast<double>((packed >> 24) & 0xff) / 255.0,
        static_cast<double>((packed >> 16) & 0xff) / 255.0,
        static_cast<double>((packed >> 8) & 0xff) / 255.0,
        static_cast<double>(packed & 0xff) / 255.0,
    };
}

document::BlendMode parse_blend_mode(QString name)
{
    if (name.startsWith("mix-blend-mode:")) {
        name.remove(0, QStringLiteral("mix-blend-mode:").size());
    }
    using Pair = std::pair<const char*, document::BlendMode>;
    static constexpr Pair modes[] = {
        {"normal", document::BlendMode::Normal}, {"multiply", document::BlendMode::Multiply},
        {"screen", document::BlendMode::Screen}, {"overlay", document::BlendMode::Overlay},
        {"darken", document::BlendMode::Darken}, {"lighten", document::BlendMode::Lighten},
        {"color-dodge", document::BlendMode::ColorDodge}, {"color-burn", document::BlendMode::ColorBurn},
        {"hard-light", document::BlendMode::HardLight}, {"soft-light", document::BlendMode::SoftLight},
        {"difference", document::BlendMode::Difference}, {"exclusion", document::BlendMode::Exclusion},
        {"hue", document::BlendMode::Hue}, {"saturation", document::BlendMode::Saturation},
        {"color", document::BlendMode::Color}, {"luminosity", document::BlendMode::Luminosity},
    };
    for (const auto& [text, mode] : modes) {
        if (name == text) return mode;
    }
    throw parse_error("Unsupported blend mode");
}

void parse_curve_metadata(
    const QXmlStreamAttributes& attributes,
    curves::PolarRoseParameters& parameters)
{
    parameters.radius = parse_double(required_attribute(attributes, "radius"), "radius");
    const auto mode = required_attribute(attributes, "k-mode");
    if (mode == "decimal") {
        parameters.k_mode = curves::PolarKMode::Decimal;
    } else if (mode == "fraction") {
        parameters.k_mode = curves::PolarKMode::Fraction;
    } else {
        throw parse_error("Unsupported polar rose k mode");
    }
    parameters.k = parse_double(required_attribute(attributes, "k"), "k");
    parameters.numerator = parse_integer(required_attribute(attributes, "numerator"), "numerator");
    parameters.denominator = parse_integer(required_attribute(attributes, "denominator"), "denominator");
    parameters.phase_degrees = parse_double(required_attribute(attributes, "phase-degrees"), "phase-degrees");
    parameters.rotation_degrees = parse_double(required_attribute(attributes, "rotation-degrees"), "rotation-degrees");
    parameters.bezier_tolerance = parse_double(required_attribute(attributes, "bezier-tolerance"), "bezier-tolerance");
}

void parse_curve_metadata(
    const QXmlStreamAttributes& attributes,
    curves::EllipseParameters& parameters)
{
    parameters.radius_x = parse_double(required_attribute(attributes, "radius-x"), "radius-x");
    parameters.radius_y = parse_double(required_attribute(attributes, "radius-y"), "radius-y");
    if (attributes.hasAttribute("link-radii")) {
        parameters.link_radii = parse_boolean(
            attributes.value("link-radii").toString(), "link-radii");
    }
    if (parameters.link_radii) {
        parameters.radius_y = parameters.radius_x;
    }
    parameters.rotation_degrees = parse_double(
        required_attribute(attributes, "rotation-degrees"), "rotation-degrees");
    parameters.bezier_tolerance = parse_double(
        required_attribute(attributes, "bezier-tolerance"), "bezier-tolerance");
}

void parse_curve_metadata(
    const QXmlStreamAttributes& attributes,
    curves::TrochoidParameters& parameters)
{
    parameters.fixed_radius = parse_double(
        required_attribute(attributes, "fixed-radius"), "fixed-radius");
    parameters.rolling_radius = parse_double(
        required_attribute(attributes, "rolling-radius"), "rolling-radius");
    parameters.pen_offset = parse_double(
        required_attribute(attributes, "pen-offset"), "pen-offset");
    parameters.rotation_degrees = parse_double(
        required_attribute(attributes, "rotation-degrees"), "rotation-degrees");
    const auto mode = required_attribute(attributes, "trace-mode");
    if (mode == "complete") parameters.trace_mode = curves::TraceMode::Complete;
    else if (mode == "limited") parameters.trace_mode = curves::TraceMode::Limited;
    else throw parse_error("Unsupported trochoid trace mode");
    parameters.turns = parse_double(required_attribute(attributes, "turns"), "turns");
    parameters.close_limited_path = parse_boolean(
        required_attribute(attributes, "close-limited-path"), "close-limited-path");
    parameters.bezier_tolerance = parse_double(
        required_attribute(attributes, "bezier-tolerance"), "bezier-tolerance");
}

void parse_curve_metadata(
    const QXmlStreamAttributes& attributes,
    curves::LissajousParameters& p)
{
    p.amplitude_x = parse_double(required_attribute(attributes, "amplitude-x"), "amplitude-x");
    p.amplitude_y = parse_double(required_attribute(attributes, "amplitude-y"), "amplitude-y");
    p.frequency_x = parse_integer(required_attribute(attributes, "frequency-x"), "frequency-x");
    p.frequency_y = parse_integer(required_attribute(attributes, "frequency-y"), "frequency-y");
    p.phase_x_degrees = parse_double(required_attribute(attributes, "phase-x-degrees"), "phase-x-degrees");
    p.phase_y_degrees = parse_double(required_attribute(attributes, "phase-y-degrees"), "phase-y-degrees");
    p.rotation_degrees = parse_double(required_attribute(attributes, "rotation-degrees"), "rotation-degrees");
    p.bezier_tolerance = parse_double(required_attribute(attributes, "bezier-tolerance"), "bezier-tolerance");
}

void parse_curve_metadata(const QXmlStreamAttributes& a, curves::HarmonographParameters& p)
{
    p.amplitude_x=parse_double(required_attribute(a,"amplitude-x"),"amplitude-x");
    p.amplitude_y=parse_double(required_attribute(a,"amplitude-y"),"amplitude-y");
    p.frequency_x=parse_double(required_attribute(a,"frequency-x"),"frequency-x");
    p.frequency_y=parse_double(required_attribute(a,"frequency-y"),"frequency-y");
    p.phase_x_degrees=parse_double(required_attribute(a,"phase-x-degrees"),"phase-x-degrees");
    p.phase_y_degrees=parse_double(required_attribute(a,"phase-y-degrees"),"phase-y-degrees");
    p.damping_x=parse_double(required_attribute(a,"damping-x"),"damping-x");
    p.damping_y=parse_double(required_attribute(a,"damping-y"),"damping-y");
    p.duration=parse_double(required_attribute(a,"duration"),"duration");
    p.rotation_degrees=parse_double(required_attribute(a,"rotation-degrees"),"rotation-degrees");
    p.bezier_tolerance=parse_double(required_attribute(a,"bezier-tolerance"),"bezier-tolerance");
}

void parse_curve_metadata(const QXmlStreamAttributes& a, curves::DropletRosetteParameters& p)
{
    p.droplets = parse_integer(required_attribute(a, "droplets"), "droplets");
    p.outer_radius = parse_double(required_attribute(a, "outer-radius"), "outer-radius");
    p.rotation_degrees = parse_double(required_attribute(a, "rotation-degrees"), "rotation-degrees");
}

void parse_path_appearance(
    const QXmlStreamAttributes& attributes,
    const QString& metadata_ns,
    document::LayerAppearance& appearance)
{
    const double stroke_alpha = parse_double(required_attribute(attributes, "stroke-opacity"), "stroke-opacity");
    const auto stroke = required_attribute(attributes, "stroke");
    appearance.stroke_enabled = stroke != "none";
    const auto stored_stroke = attributes.value(metadata_ns, "stroke-color");
    appearance.stroke = parse_rgb(
        !stored_stroke.isNull() ? stored_stroke.toString() : stroke,
        optional_metadata_double(attributes, metadata_ns, "stroke-alpha", stroke_alpha));
    appearance.stroke_width = parse_double(required_attribute(attributes, "stroke-width"), "stroke-width");

    const auto fill = required_attribute(attributes, "fill");
    appearance.fill_enabled = fill != "none";
    const double fill_alpha = appearance.fill_enabled
        ? parse_double(required_attribute(attributes, "fill-opacity"), "fill-opacity") : 1.0;
    const auto stored_fill = attributes.value(metadata_ns, "fill-color");
    if (appearance.fill_enabled || !stored_fill.isNull())
        appearance.fill = parse_rgb(
            !stored_fill.isNull() ? stored_fill.toString() : fill,
            optional_metadata_double(attributes, metadata_ns, "fill-alpha", fill_alpha));
    const auto rule = required_attribute(attributes, "fill-rule");
    if (rule == "nonzero") appearance.fill_rule = document::FillRule::NonZero;
    else if (rule == "evenodd") appearance.fill_rule = document::FillRule::EvenOdd;
    else throw parse_error("Unsupported fill rule");

    appearance.opacity = parse_double(required_attribute(attributes, "opacity"), "opacity");
    appearance.blend_mode = parse_blend_mode(required_attribute(attributes, "style"));
}

document::CurveLayer parse_layer(QXmlStreamReader& reader, const QString& metadata_ns)
{
    const auto group_attributes = reader.attributes();
    document::CurveLayer layer;
    bool id_ok = false;
    layer.id = required_metadata_attribute(group_attributes, metadata_ns, "id").toULongLong(&id_ok);
    if (!id_ok || layer.id == 0) throw parse_error("Invalid layer ID");
    layer.name = required_metadata_attribute(group_attributes, metadata_ns, "name").toStdString();
    const auto type = required_metadata_attribute(group_attributes, metadata_ns, "type");
    if (type == "polar-rose") layer.type = document::CurveType::PolarRose;
    else if (type == "ellipse") layer.type = document::CurveType::Ellipse;
    else if (type == "hypotrochoid") layer.type = document::CurveType::Hypotrochoid;
    else if (type == "epitrochoid") layer.type = document::CurveType::Epitrochoid;
    else if (type == "lissajous") layer.type = document::CurveType::Lissajous;
    else if (type == "harmonograph") layer.type = document::CurveType::Harmonograph;
    else if (type == "droplet-rosette") layer.type = document::CurveType::DropletRosette;
    else throw parse_error("Unsupported RosetteLab curve type");
    layer.visible = parse_boolean(
        required_metadata_attribute(group_attributes, metadata_ns, "visible"), "visible");
    layer.locked = parse_boolean(
        required_metadata_attribute(group_attributes, metadata_ns, "locked"), "locked");
    layer.transform.position_x = optional_metadata_double(
        group_attributes, metadata_ns, "position-x", 0.0);
    layer.transform.position_y = optional_metadata_double(
        group_attributes, metadata_ns, "position-y", 0.0);
    layer.transform.scale_x = optional_metadata_double(
        group_attributes, metadata_ns, "scale-x", 1.0);
    layer.transform.scale_y = optional_metadata_double(
        group_attributes, metadata_ns, "scale-y", 1.0);
    layer.transform.link_scales = optional_metadata_boolean(
        group_attributes, metadata_ns, "link-scales", true);
    layer.transform.rotation_degrees = optional_metadata_double(
        group_attributes, metadata_ns, "layer-rotation-degrees", 0.0);
    const auto arrangement = group_attributes.value(metadata_ns, "copy-arrangement");
    if (arrangement.isNull()) {
        const double legacy_offset_x = optional_metadata_double(
            group_attributes, metadata_ns, "copy-offset-x", 0.0);
        const double legacy_offset_y = optional_metadata_double(
            group_attributes, metadata_ns, "copy-offset-y", 0.0);
        layer.copies.arrangement = legacy_offset_x == 0.0 && legacy_offset_y == 0.0
            ? document::CopyArrangement::Superimposed
            : document::CopyArrangement::Linear;
    } else if (arrangement == "superimposed") {
        layer.copies.arrangement = document::CopyArrangement::Superimposed;
    } else if (arrangement == "linear") {
        layer.copies.arrangement = document::CopyArrangement::Linear;
    } else if (arrangement == "circular") {
        layer.copies.arrangement = document::CopyArrangement::Circular;
    } else {
        throw parse_error("Unsupported copy arrangement");
    }
    layer.copies.count = optional_metadata_integer(
        group_attributes, metadata_ns, "copy-count", 1);
    layer.copies.rotation_step_degrees = optional_metadata_double(
        group_attributes, metadata_ns, "copy-rotation-degrees", 0.0);
    layer.copies.scale_step = optional_metadata_double(
        group_attributes, metadata_ns, "copy-scale-step", 1.0);
    layer.copies.offset_x_step = optional_metadata_double(
        group_attributes, metadata_ns, "copy-offset-x", 0.0);
    layer.copies.offset_y_step = optional_metadata_double(
        group_attributes, metadata_ns, "copy-offset-y", 0.0);
    layer.copies.circular_radius = optional_metadata_double(
        group_attributes, metadata_ns, "copy-circular-radius", 0.0);
    layer.copies.circular_start_degrees = optional_metadata_double(
        group_attributes, metadata_ns, "copy-circular-start-degrees", 0.0);
    layer.copies.circular_angle_step_degrees = optional_metadata_double(
        group_attributes, metadata_ns, "copy-circular-angle-degrees", 0.0);
    layer.copies.rotate_with_orbit = optional_metadata_boolean(
        group_attributes, metadata_ns, "copy-rotate-with-orbit", true);
    auto& palette = layer.appearance.cyclic_palette;
    palette.enabled = optional_metadata_boolean(
        group_attributes, metadata_ns, "cyclic-palette-enabled", false);
    const auto palette_scope = group_attributes.value(metadata_ns, "cyclic-palette-scope");
    if (!palette_scope.isNull()) {
        if (palette_scope == "subpaths") palette.scope = document::PaletteScope::Subpaths;
        else if (palette_scope == "copies") palette.scope = document::PaletteScope::Copies;
        else throw parse_error("Unsupported cyclic palette scope");
    }
    const auto palette_target = group_attributes.value(metadata_ns, "cyclic-palette-target");
    if (!palette_target.isNull()) {
        if (palette_target == "fill") palette.target = document::PaletteTarget::Fill;
        else if (palette_target == "stroke") palette.target = document::PaletteTarget::Stroke;
        else if (palette_target == "fill-and-stroke") palette.target = document::PaletteTarget::FillAndStroke;
        else throw parse_error("Unsupported cyclic palette target");
    }
    palette.offset = optional_metadata_integer(
        group_attributes, metadata_ns, "cyclic-palette-offset", 0);
    const auto palette_colors = group_attributes.value(metadata_ns, "cyclic-palette-colors");
    if (!palette_colors.isNull() && !palette_colors.isEmpty()) {
        for (const auto& color : palette_colors.toString().split(',')) {
            palette.colors.push_back(parse_rgba(color));
        }
    }
    if (layer.transform.scale_x <= 0.0 || layer.transform.scale_y <= 0.0 ||
        layer.copies.count < 1 || layer.copies.count > 1000 || layer.copies.scale_step <= 0.0 ||
        layer.copies.circular_radius < 0.0) {
        throw parse_error("Invalid layer transform or copy settings");
    }
    const auto preset_id = group_attributes.value(metadata_ns, "preset-id");
    if (!preset_id.isNull()) {
        layer.preset_id = preset_id.toString().toStdString();
        const auto customized = group_attributes.value(metadata_ns, "preset-customized");
        layer.preset_customized = !customized.isNull()
            && parse_boolean(customized.toString(), "preset-customized");
    }

    document::CurveParameters parameters;
    if (layer.type == document::CurveType::PolarRose) {
        parameters = curves::PolarRoseParameters{};
    } else if (layer.type == document::CurveType::Ellipse) {
        parameters = curves::EllipseParameters{};
    } else if (layer.type == document::CurveType::Lissajous) {
        parameters = curves::LissajousParameters{};
    } else if (layer.type == document::CurveType::Harmonograph) {
        parameters = curves::HarmonographParameters{};
    } else if (layer.type == document::CurveType::DropletRosette) {
        parameters = curves::DropletRosetteParameters{};
    } else {
        parameters = curves::TrochoidParameters{};
    }
    bool found_curve = false;
    bool found_path = false;
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() == metadata_ns && reader.name() == "curve") {
            std::visit([&reader](auto& value) {
                parse_curve_metadata(reader.attributes(), value);
            }, parameters);
            found_curve = true;
            reader.skipCurrentElement();
        } else if (reader.namespaceUri() == "http://www.w3.org/2000/svg" && reader.name() == "path") {
            if (!found_path) {
                parse_path_appearance(reader.attributes(), metadata_ns, layer.appearance);
                found_path = true;
            }
            reader.skipCurrentElement();
        } else {
            reader.skipCurrentElement();
        }
    }
    if (!found_curve || !found_path) {
        throw parse_error("RosetteLab layer is missing curve metadata or rendered path");
    }
    layer.parameters = parameters;
    return layer;
}

} // namespace

document::Document parse_rosettelab_svg(const QByteArray& data)
{
    if (data.size() > 100 * 1024 * 1024) {
        throw parse_error("RosetteLab SVG exceeds the 100 MB safety limit");
    }
    const auto lowered = data.toLower();
    if (lowered.contains("<!doctype") || lowered.contains("<!entity")) {
        throw parse_error("DTD and entity declarations are not allowed");
    }

    QXmlStreamReader reader(data);
    if (!reader.readNextStartElement() ||
        reader.namespaceUri() != "http://www.w3.org/2000/svg" || reader.name() != "svg") {
        throw parse_error("File is not an SVG document");
    }

    const QString metadata_ns = QString::fromUtf8(metadata_namespace);
    const auto attributes = reader.attributes();
    if (required_metadata_attribute(attributes, metadata_ns, "document") != "true") {
        throw parse_error("SVG is not a RosetteLab project");
    }
    if (required_metadata_attribute(attributes, metadata_ns, "schema-version") != schema_version) {
        throw parse_error("Unsupported RosetteLab schema version");
    }

    document::Document document;
    document.settings().page_width = parse_double(
        required_metadata_attribute(attributes, metadata_ns, "page-width"), "page-width");
    document.settings().page_height = parse_double(
        required_metadata_attribute(attributes, metadata_ns, "page-height"), "page-height");
    document.settings().unit = required_metadata_attribute(attributes, metadata_ns, "unit").toStdString();
    const double background_alpha = parse_double(
        required_metadata_attribute(attributes, metadata_ns, "background-opacity"),
        "background-opacity");
    document.settings().background = parse_rgb(
        required_metadata_attribute(attributes, metadata_ns, "background"),
        background_alpha);
    if (document.settings().page_width <= 0.0 || document.settings().page_height <= 0.0 ||
        document.settings().unit.empty()) {
        throw parse_error("Invalid RosetteLab page settings");
    }
    std::size_t layer_count = 0;
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() == "http://www.w3.org/2000/svg" && reader.name() == "g") {
            if (++layer_count > 10'000) {
                throw parse_error("RosetteLab SVG contains too many layers");
            }
            auto layer = parse_layer(reader, metadata_ns);
            if (!document.import_layer(std::move(layer))) {
                throw parse_error("Invalid or duplicate RosetteLab layer");
            }
        } else {
            reader.skipCurrentElement();
        }
    }
    if (reader.hasError()) {
        throw parse_error(QStringLiteral("Invalid XML: %1").arg(reader.errorString()));
    }
    return document;
}

} // namespace rosettelab::svg
