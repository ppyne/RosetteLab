#pragma once

#include "rosettelab/curves/polar_rose.hpp"
#include "rosettelab/document/document.hpp"

#include <QWidget>
#include <QSizeF>

namespace rosettelab::ui {

class PreviewWidget final : public QWidget {
public:
    explicit PreviewWidget(QWidget* parent = nullptr);

    void set_document(const document::Document* document);
    void set_zoom_percent(double zoom_percent);
    void refresh_document_geometry();
    [[nodiscard]] double fit_zoom_percent(const QSizeF& available_size) const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    const document::Document* document_{};
    double zoom_percent_{100.0};
    double pixels_per_unit_{2.5};

    void update_canvas_size();
};

} // namespace rosettelab::ui
