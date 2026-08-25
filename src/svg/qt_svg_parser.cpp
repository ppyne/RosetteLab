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

void parse_path_appearance(
    const QXmlStreamAttributes& attributes,
    document::LayerAppearance& appearance)
{
    const double stroke_alpha = parse_double(required_attribute(attributes, "stroke-opacity"), "stroke-opacity");
    appearance.stroke = parse_rgb(required_attribute(attributes, "stroke"), stroke_alpha);
    appearance.stroke_width = parse_double(required_attribute(attributes, "stroke-width"), "stroke-width");

    const auto fill = required_attribute(attributes, "fill");
    appearance.fill_enabled = fill != "none";
    if (appearance.fill_enabled) {
        const double fill_alpha = parse_double(required_attribute(attributes, "fill-opacity"), "fill-opacity");
        appearance.fill = parse_rgb(fill, fill_alpha);
    }
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
    else throw parse_error("Unsupported RosetteLab curve type");
    layer.visible = parse_boolean(
        required_metadata_attribute(group_attributes, metadata_ns, "visible"), "visible");
    layer.locked = parse_boolean(
        required_metadata_attribute(group_attributes, metadata_ns, "locked"), "locked");

    document::CurveParameters parameters;
    if (layer.type == document::CurveType::PolarRose) {
        parameters = curves::PolarRoseParameters{};
    } else if (layer.type == document::CurveType::Ellipse) {
        parameters = curves::EllipseParameters{};
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
            parse_path_appearance(reader.attributes(), layer.appearance);
            found_path = true;
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
