#include "ui/preview_widget.hpp"

#include <QPainter>
#include <QPainterPath>

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

void PreviewWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // The application chrome follows the operating-system theme, while the
    // document canvas has its own explicit appearance.
    painter.fillRect(rect(), Qt::white);

    core::BezierPath curve;
    try {
        curve = curves::generate_polar_rose_bezier(parameters_);
    } catch (const std::exception&) {
        return;
    }

    if (curve.segments.empty()) {
        return;
    }

    const double diameter = parameters_.radius * 2.0;
    const double available = 0.86 * static_cast<double>(std::min(width(), height()));
    const double scale = diameter > 0.0 ? available / diameter : 1.0;

    QPainterPath path;
    const auto& first = curve.segments.front().start;
    path.moveTo(first.x * scale, first.y * scale);
    for (const auto& segment : curve.segments) {
        path.cubicTo(
            segment.control1.x * scale,
            segment.control1.y * scale,
            segment.control2.x * scale,
            segment.control2.y * scale,
            segment.end.x * scale,
            segment.end.y * scale);
    }
    if (curve.closed) {
        path.closeSubpath();
    }

    painter.translate(width() / 2.0, height() / 2.0);
    QPen pen(Qt::black);
    pen.setWidthF(1.5);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

} // namespace rosettelab::ui
