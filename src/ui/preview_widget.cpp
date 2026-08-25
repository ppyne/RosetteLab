#include "ui/preview_widget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPalette>

#include <algorithm>
#include <exception>

namespace rosettelab::ui {

PreviewWidget::PreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(360, 360);
    setAutoFillBackground(true);
}

void PreviewWidget::set_parameters(const curves::PolarRoseParameters& parameters)
{
    parameters_ = parameters;
    update();
}

void PreviewWidget::set_zoom_percent(const double zoom_percent)
{
    zoom_percent_ = std::clamp(zoom_percent, 10.0, 800.0);
    update();
}

void PreviewWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), palette().brush(QPalette::Window));

    const double fit_scale = 0.9 * std::min(
        static_cast<double>(width()) / page_width_,
        static_cast<double>(height()) / page_height_);
    const double document_scale = fit_scale * zoom_percent_ / 100.0;
    const QSizeF page_size(page_width_ * document_scale, page_height_ * document_scale);
    const QRectF page_rect(
        (width() - page_size.width()) / 2.0,
        (height() - page_size.height()) / 2.0,
        page_size.width(),
        page_size.height());

    painter.setPen(QPen(QColor(0, 0, 0, 45), 1.0));
    painter.setBrush(Qt::white);
    painter.drawRect(page_rect);
    core::BezierPath curve;
    try {
        curve = curves::generate_polar_rose_bezier(parameters_);
    } catch (const std::exception&) {
        return;
    }

    if (curve.segments.empty()) {
        return;
    }

    painter.save();
    painter.setClipRect(page_rect.adjusted(1.0, 1.0, -1.0, -1.0));

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

    painter.translate(page_rect.center());
    QPen pen(Qt::black);
    pen.setWidthF(1.5);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
    painter.restore();
}

} // namespace rosettelab::ui
