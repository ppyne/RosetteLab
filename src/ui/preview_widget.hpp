#pragma once

#include "rosettelab/curves/polar_rose.hpp"

#include <QWidget>

namespace rosettelab::ui {

class PreviewWidget final : public QWidget {
public:
    explicit PreviewWidget(QWidget* parent = nullptr);

    void set_parameters(const curves::PolarRoseParameters& parameters);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    curves::PolarRoseParameters parameters_;
};

} // namespace rosettelab::ui

