#include "ui/preview_widget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPalette>

#include <algorithm>
#include <cmath>
#include <exception>

namespace rosettelab::ui {
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
    // QPainter has no HSL blend modes. They remain part of the document/SVG
    // model but are not offered by the Qt preview until a custom compositor
    // is implemented.
    case Hue:
    case Saturation:
    case Color:
    case Luminosity:
        return QPainter::CompositionMode_SourceOver;
    }
    return QPainter::CompositionMode_SourceOver;
}

} // namespace

PreviewWidget::PreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    update_canvas_size();
}

void PreviewWidget::set_document(const document::Document* document)
{
    document_ = document;
    update_canvas_size();
}

void PreviewWidget::refresh_document_geometry()
{
    update_canvas_size();
}

void PreviewWidget::set_zoom_percent(const double zoom_percent)
{
    zoom_percent_ = std::clamp(zoom_percent, 10.0, 800.0);
    update_canvas_size();
}

void PreviewWidget::update_canvas_size()
{
    constexpr double margin = 40.0;
    const double scale = pixels_per_unit_ * zoom_percent_ / 100.0;
    const double page_width = document_ != nullptr ? document_->settings().page_width : 210.0;
    const double page_height = document_ != nullptr ? document_->settings().page_height : 210.0;
    setFixedSize(
        static_cast<int>(std::ceil(page_width * scale + margin)),
        static_cast<int>(std::ceil(page_height * scale + margin)));
    update();
}

void PreviewWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), palette().brush(QPalette::Window));

    const double document_scale = pixels_per_unit_ * zoom_percent_ / 100.0;
    const double page_width = document_ != nullptr ? document_->settings().page_width : 210.0;
    const double page_height = document_ != nullptr ? document_->settings().page_height : 210.0;
    const QSizeF page_size(page_width * document_scale, page_height * document_scale);
    const QRectF page_rect(
        (width() - page_size.width()) / 2.0,
        (height() - page_size.height()) / 2.0,
        page_size.width(),
        page_size.height());

    painter.setPen(QPen(QColor(0, 0, 0, 45), 1.0));
    painter.setBrush(document_ != nullptr
        ? QBrush(to_qcolor(document_->settings().background))
        : QBrush(Qt::white));
    painter.drawRect(page_rect);
    if (document_ == nullptr) {
        return;
    }

    painter.save();
    painter.setClipRect(page_rect.adjusted(1.0, 1.0, -1.0, -1.0));

    painter.translate(page_rect.center());
    for (const auto& layer : document_->layers()) {
        if (!layer.visible) {
            continue;
        }
        const auto* parameters = std::get_if<curves::PolarRoseParameters>(&layer.parameters);
        if (parameters == nullptr) {
            continue;
        }

        core::BezierPath curve;
        try {
            curve = curves::generate_polar_rose_bezier(*parameters, parameters->bezier_tolerance);
        } catch (const std::exception&) {
            continue;
        }
        if (curve.segments.empty()) {
            continue;
        }

        QPainterPath path;
        const auto& first = curve.segments.front().start;
        path.moveTo(first.x * document_scale, first.y * document_scale);
        for (const auto& segment : curve.segments) {
            path.cubicTo(
                segment.control1.x * document_scale,
                segment.control1.y * document_scale,
                segment.control2.x * document_scale,
                segment.control2.y * document_scale,
                segment.end.x * document_scale,
                segment.end.y * document_scale);
        }
        if (curve.closed) {
            path.closeSubpath();
        }
        path.setFillRule(layer.appearance.fill_rule == document::FillRule::EvenOdd
            ? Qt::OddEvenFill
            : Qt::WindingFill);

        painter.save();
        painter.setOpacity(std::clamp(layer.appearance.opacity, 0.0, 1.0));
        painter.setCompositionMode(composition_mode(layer.appearance.blend_mode));
        QPen pen(to_qcolor(layer.appearance.stroke));
        pen.setWidthF(std::max(0.0, layer.appearance.stroke_width * document_scale));
        painter.setPen(pen);
        painter.setBrush(layer.appearance.fill_enabled
            ? QBrush(to_qcolor(layer.appearance.fill))
            : QBrush(Qt::NoBrush));
        painter.drawPath(path);
        painter.restore();
    }
    painter.restore();
}

} // namespace rosettelab::ui
