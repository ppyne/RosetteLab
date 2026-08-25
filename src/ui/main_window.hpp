#pragma once

#include "rosettelab/curves/polar_rose.hpp"
#include "rosettelab/document/document.hpp"

#include <QMainWindow>

class QDoubleSpinBox;
class QGroupBox;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSpinBox;

namespace rosettelab::ui {

class PreviewWidget;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void add_polar_rose();
    QListWidgetItem* add_layer_row(const document::CurveLayer& layer, int row = -1);
    void select_layer_row(document::LayerId id);
    void select_layer(document::LayerId id);
    void load_active_layer();
    void rename_active_layer();
    void duplicate_active_layer();
    void delete_active_layer();
    void set_active_layer_locked(bool locked);
    void refresh_layer_actions();
    void sync_layer_order();
    void update_preview();

    PreviewWidget* preview_{};
    QDoubleSpinBox* radius_{};
    QDoubleSpinBox* k_{};
    QDoubleSpinBox* phase_{};
    QDoubleSpinBox* rotation_{};
    QDoubleSpinBox* tolerance_{};
    QSpinBox* zoom_{};
    QGroupBox* curve_group_{};
    QListWidget* layers_{};
    QPushButton* rename_button_{};
    QPushButton* duplicate_button_{};
    QPushButton* delete_button_{};
    document::Document document_;
    document::LayerId active_layer_id_{};
};

} // namespace rosettelab::ui
