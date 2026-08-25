#include "ui/preview_widget.hpp"
#include "render/document_renderer.hpp"

#include <QPainter>
#include <QPalette>

#include <algorithm>
#include <cmath>

namespace rosettelab::ui {
namespace {

QColor to_qcolor(const document::RgbaColor& color)
{
    return QColor::fromRgbF(color.red, color.green, color.blue, color.alpha);
}

void draw_checkerboard(QPainter& painter, const QRectF& area)
{
    constexpr int square = 12;
    painter.save();
    painter.setClipRect(area);
    const int left = static_cast<int>(std::floor(area.left()));
    const int top = static_cast<int>(std::floor(area.top()));
    const int right = static_cast<int>(std::ceil(area.right()));
    const int bottom = static_cast<int>(std::ceil(area.bottom()));
    for (int y = top; y < bottom; y += square) {
        for (int x = left; x < right; x += square) {
            const bool grey = ((x - left) / square + (y - top) / square) % 2 != 0;
            painter.fillRect(
                QRect(x, y, square, square),
                grey ? QColor(127, 127, 127) : QColor(255, 255, 255));
        }
    }
    painter.restore();
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

    const auto page_background = document_ != nullptr
        ? to_qcolor(document_->settings().background)
        : QColor(Qt::white);
    if (page_background.alpha() < 255) {
        draw_checkerboard(painter, page_rect);
    }
    if (document_ != nullptr) {
        render::render_document(painter, *document_, page_rect);
    } else {
        painter.fillRect(page_rect, page_background);
    }
    painter.setPen(QPen(QColor(0, 0, 0, 45), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(page_rect);
    if (document_ == nullptr) {
        return;
    }
}

} // namespace rosettelab::ui
