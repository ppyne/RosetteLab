#pragma once

#include "rosettelab/curves/polar_rose.hpp"

#include <QMainWindow>

class QDoubleSpinBox;
class QListWidgetItem;
class QSpinBox;

namespace rosettelab::ui {

class PreviewWidget;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void update_preview();

    PreviewWidget* preview_{};
    QDoubleSpinBox* radius_{};
    QDoubleSpinBox* k_{};
    QDoubleSpinBox* phase_{};
    QDoubleSpinBox* rotation_{};
    QDoubleSpinBox* tolerance_{};
    QSpinBox* zoom_{};
    QListWidgetItem* active_layer_item_{};
    bool active_layer_has_custom_name_{false};
};

} // namespace rosettelab::ui
