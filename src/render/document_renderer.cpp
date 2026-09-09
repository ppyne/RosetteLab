#include "render/document_renderer.hpp"
#include "render/text_support.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetricsF>
#include <QTransform>

#include <algorithm>
#include <exception>

namespace rosettelab::render {
namespace {

QColor to_qcolor(const document::RgbaColor& color)
{
    return QColor::fromRgbF(color.red, color.green, color.blue, color.alpha);
}

QPainter::CompositionMode composition_mode(const document::BlendMode mode)
{
    using enum document::BlendMode;
    switch (mode) {
    case Normal: return QPainter::CompositionMode_SourceOver;
    case Multiply: return QPainter::CompositionMode_Multiply;
    case Screen: return QPainter::CompositionMode_Screen;
    case Overlay: return QPainter::CompositionMode_Overlay;
    case Darken: return QPainter::CompositionMode_Darken;
    case Lighten: return QPainter::CompositionMode_Lighten;
    case ColorDodge: return QPainter::CompositionMode_ColorDodge;
    case ColorBurn: return QPainter::CompositionMode_ColorBurn;
    case HardLight: return QPainter::CompositionMode_HardLight;
    case SoftLight: return QPainter::CompositionMode_SoftLight;
    case Difference: return QPainter::CompositionMode_Difference;
    case Exclusion: return QPainter::CompositionMode_Exclusion;
    case Hue:
    case Saturation:
    case Color:
    case Luminosity:
        return QPainter::CompositionMode_SourceOver;
    }
    return QPainter::CompositionMode_SourceOver;
}

core::BezierPath layer_path(const document::CurveLayer& layer)
{
    if (const auto* parameters = std::get_if<curves::PolarRoseParameters>(&layer.parameters)) {
        return curves::generate_polar_rose_bezier(*parameters, parameters->bezier_tolerance);
    }
    if (const auto* parameters = std::get_if<curves::EllipseParameters>(&layer.parameters)) {
        return curves::generate_ellipse_bezier(*parameters, parameters->bezier_tolerance);
    }
    if (const auto* parameters = std::get_if<curves::TrochoidParameters>(&layer.parameters)) {
        const auto kind = layer.type == document::CurveType::Hypotrochoid
            ? curves::TrochoidKind::Hypotrochoid
            : curves::TrochoidKind::Epitrochoid;
        return curves::generate_trochoid_bezier(kind, *parameters, parameters->bezier_tolerance);
    }
    if (const auto* parameters = std::get_if<curves::LissajousParameters>(&layer.parameters)) {
        return curves::generate_lissajous_bezier(*parameters, parameters->bezier_tolerance);
    }
    if (const auto* parameters = std::get_if<curves::HarmonographParameters>(&layer.parameters)) {
        return curves::generate_harmonograph_bezier(*parameters, parameters->bezier_tolerance);
    }
    if (const auto* parameters = std::get_if<curves::DropletRosetteParameters>(&layer.parameters)) {
        return curves::generate_droplet_rosette_bezier(*parameters);
    }
    if (const auto* parameters = std::get_if<document::ImportedSvgParameters>(&layer.parameters)) {
        return parameters->geometry;
    }
    return {};
}

QPainterPath painter_path(const core::BezierPath& curve, const document::FillRule fill_rule)
{
    QPainterPath path;
    for (std::size_t index = 0; index < curve.segments.size(); ++index) {
        const auto& segment = curve.segments[index];
        if (index == 0 || std::find(
                curve.subpath_starts.begin(), curve.subpath_starts.end(), index) != curve.subpath_starts.end()) {
            const auto subpath = static_cast<std::size_t>(std::distance(
                curve.subpath_starts.begin(), std::lower_bound(
                    curve.subpath_starts.begin(), curve.subpath_starts.end(), index)));
            if (index != 0 && core::subpath_is_closed(curve, subpath - 1)) path.closeSubpath();
            path.moveTo(segment.start.x, segment.start.y);
        }
        path.cubicTo(
            segment.control1.x, segment.control1.y,
            segment.control2.x, segment.control2.y,
            segment.end.x, segment.end.y);
    }
    const std::size_t subpath_count = curve.subpath_starts.empty() ? 1 : curve.subpath_starts.size();
    if (core::subpath_is_closed(curve, subpath_count - 1)) path.closeSubpath();
    path.setFillRule(fill_rule == document::FillRule::EvenOdd ? Qt::OddEvenFill : Qt::WindingFill);
    return path;
}

void set_appearance(QPainter& painter, const document::LayerAppearance& appearance)
{
    if (appearance.stroke_enabled) {
        QPen pen(to_qcolor(appearance.stroke));
        pen.setWidthF(std::max(0.0, appearance.stroke_width));
        painter.setPen(pen);
    } else {
        painter.setPen(Qt::NoPen);
    }
    painter.setBrush(appearance.fill_enabled
        ? QBrush(to_qcolor(appearance.fill))
        : QBrush(Qt::NoBrush));
}

} // namespace

bool has_visible_blend_modes(const document::Document& document)
{
    return std::any_of(
        document.layers().begin(), document.layers().end(),
        [](const document::CurveLayer& layer) {
            return layer.visible
                && layer.appearance.blend_mode != document::BlendMode::Normal;
        });
}

void render_document(
    QPainter& painter,
    const document::Document& document,
    const QRectF& page_rect)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setClipRect(page_rect);
    painter.fillRect(page_rect, to_qcolor(document.settings().background));

    const double scale = std::min(
        page_rect.width() / document.settings().page_width,
        page_rect.height() / document.settings().page_height);
    painter.translate(page_rect.center());
    painter.scale(scale, scale);

    for (const auto& layer : document.layers()) {
        if (!layer.visible) {
            continue;
        }
        if (const auto* text = std::get_if<document::TextParameters>(&layer.parameters)) {
            painter.save();
            painter.setOpacity(std::clamp(layer.appearance.opacity, 0.0, 1.0));
            painter.setCompositionMode(composition_mode(layer.appearance.blend_mode));
            painter.translate(layer.transform.position_x, layer.transform.position_y);
            painter.rotate(layer.transform.rotation_degrees);
            painter.scale(layer.transform.scale_x, layer.transform.scale_y);
            if (text->vectorize && !text->outline.segments.empty()) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(to_qcolor(text->color));
                painter.drawPath(painter_path(text->outline, document::FillRule::NonZero));
                painter.restore();
                continue;
            }
            QFont font(QString::fromUtf8(text->font_family.data(), static_cast<qsizetype>(text->font_family.size())));
            font.setPixelSize(std::max(1, static_cast<int>(std::lround(text->font_size))));
            painter.setFont(font);
            painter.setPen(to_qcolor(text->color));
            const auto& stored_value = text->rendered_text.empty() ? text->text : text->rendered_text;
            const QString value = replace_unsupported_glyphs(
                QString::fromUtf8(stored_value.data(), static_cast<qsizetype>(stored_value.size())), font);
            const double width = QFontMetricsF(font).horizontalAdvance(value);
            double x = 0.0;
            if (text->alignment == document::TextAlignment::Center) x = -width / 2.0;
            else if (text->alignment == document::TextAlignment::Right) x = -width;
            painter.drawText(QPointF(x, 0.0), value);
            painter.restore();
            continue;
        }
        core::BezierPath curve;
        try {
            curve = layer_path(layer);
        } catch (const std::exception&) {
            continue;
        }
        if (curve.segments.empty()) {
            continue;
        }

        painter.save();
        painter.setOpacity(std::clamp(layer.appearance.opacity, 0.0, 1.0));
        painter.setCompositionMode(composition_mode(layer.appearance.blend_mode));
        const auto whole_path = painter_path(curve, layer.appearance.fill_rule);
        std::vector<QPainterPath> subpaths;
        if (layer.appearance.cyclic_palette.enabled &&
            layer.appearance.cyclic_palette.scope == document::PaletteScope::Subpaths) {
            for (const auto& part : core::split_subpaths(curve)) {
                subpaths.push_back(painter_path(part, layer.appearance.fill_rule));
            }
        }
        const int copy_count = std::clamp(layer.copies.count, 1, 1000);
        for (int copy = 0; copy < copy_count; ++copy) {
            const auto placement = document::copy_placement(layer, copy);
            QTransform transform;
            transform.translate(placement.position_x, placement.position_y);
            transform.rotate(placement.rotation_degrees);
            transform.scale(
                layer.transform.scale_x * placement.scale,
                layer.transform.scale_y * placement.scale);
            if (!subpaths.empty()) {
                for (std::size_t part = 0; part < subpaths.size(); ++part) {
                    const auto appearance = document::appearance_for_palette_index(layer.appearance, part);
                    set_appearance(painter, appearance);
                    painter.drawPath(transform.map(subpaths[part]));
                }
            } else {
                const auto appearance = document::appearance_for_palette_index(
                    layer.appearance, static_cast<std::size_t>(copy));
                set_appearance(painter, appearance);
                painter.drawPath(transform.map(whole_path));
            }
        }
        painter.restore();
    }
    painter.restore();
}

} // namespace rosettelab::render
