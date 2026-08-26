#pragma once

#include "rosettelab/curves/polar_rose.hpp"
#include "rosettelab/document/document.hpp"

#include <QWidget>
#include <QSizeF>

#include <functional>

class QMouseEvent;
class QWheelEvent;

namespace rosettelab::ui {

class PreviewWidget final : public QWidget {
public:
    explicit PreviewWidget(QWidget* parent = nullptr);

    void set_document(const document::Document* document);
    void set_zoom_percent(double zoom_percent);
    void refresh_document_geometry();
    [[nodiscard]] double fit_zoom_percent(const QSizeF& available_size) const;
    void set_wheel_zoom_handler(std::function<void(int, const QPoint&)> handler);
    void set_pan_handler(std::function<void(const QPoint&)> handler);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    const document::Document* document_{};
    double zoom_percent_{100.0};
    double pixels_per_unit_{2.5};
    std::function<void(int, const QPoint&)> wheel_zoom_handler_;
    std::function<void(const QPoint&)> pan_handler_;
    bool panning_{false};
    QPoint last_pan_position_;

    void update_canvas_size();
};

} // namespace rosettelab::ui
