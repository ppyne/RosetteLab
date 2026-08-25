#pragma once

#include "rosettelab/curves/polar_rose.hpp"

#include <QWidget>

namespace rosettelab::ui {

class PreviewWidget final : public QWidget {
public:
    explicit PreviewWidget(QWidget* parent = nullptr);

    void set_parameters(const curves::PolarRoseParameters& parameters);
    void set_zoom_percent(double zoom_percent);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    curves::PolarRoseParameters parameters_;
    double zoom_percent_{100.0};
    double page_width_{210.0};
    double page_height_{210.0};
};

} // namespace rosettelab::ui
