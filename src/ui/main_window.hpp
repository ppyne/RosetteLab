#pragma once

#include "rosettelab/curves/polar_rose.hpp"
#include "rosettelab/document/document.hpp"

#include <QMainWindow>

class QDoubleSpinBox;
class QListWidget;
class QSpinBox;

namespace rosettelab::ui {

class PreviewWidget;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void add_polar_rose();
    void select_layer(document::LayerId id);
    void load_active_layer();
    void sync_layer_order();
    void update_preview();

    PreviewWidget* preview_{};
    QDoubleSpinBox* radius_{};
    QDoubleSpinBox* k_{};
    QDoubleSpinBox* phase_{};
    QDoubleSpinBox* rotation_{};
    QDoubleSpinBox* tolerance_{};
    QSpinBox* zoom_{};
    QListWidget* layers_{};
    document::Document document_;
    document::LayerId active_layer_id_{};
};

} // namespace rosettelab::ui
