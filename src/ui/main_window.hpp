#pragma once

#include "rosettelab/curves/polar_rose.hpp"
#include "rosettelab/document/document.hpp"

#include <QColor>
#include <QMainWindow>

class QCheckBox;
class QComboBox;
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
    void open_file();
    void save_as();
    void rebuild_layer_list();
    QListWidgetItem* add_layer_row(const document::CurveLayer& layer, int row = -1);
    void select_layer_row(document::LayerId id);
    void select_layer(document::LayerId id);
    void load_active_layer();
    void rename_active_layer();
    void duplicate_active_layer();
    void delete_active_layer();
    void set_active_layer_locked(bool locked);
    void choose_stroke_color();
    void choose_fill_color();
    void choose_page_background();
    void update_appearance();
    void update_document_settings();
    void load_document_settings();
    void refresh_k_mode_controls();
    void refresh_color_buttons();
    void refresh_layer_actions();
    void sync_layer_order();
    void update_preview();

    PreviewWidget* preview_{};
    QDoubleSpinBox* page_width_{};
    QDoubleSpinBox* page_height_{};
    QPushButton* page_background_button_{};
    QColor page_background_{Qt::white};
    QDoubleSpinBox* radius_{};
    QComboBox* k_mode_{};
    QDoubleSpinBox* k_{};
    QSpinBox* numerator_{};
    QSpinBox* denominator_{};
    QDoubleSpinBox* phase_{};
    QDoubleSpinBox* rotation_{};
    QDoubleSpinBox* tolerance_{};
    QSpinBox* zoom_{};
    QGroupBox* curve_group_{};
    QGroupBox* appearance_group_{};
    QPushButton* stroke_color_button_{};
    QPushButton* fill_color_button_{};
    QDoubleSpinBox* stroke_width_{};
    QCheckBox* fill_enabled_{};
    QComboBox* fill_rule_{};
    QSpinBox* layer_opacity_{};
    QComboBox* blend_mode_{};
    QColor stroke_color_{Qt::black};
    QColor fill_color_{Qt::white};
    QListWidget* layers_{};
    QPushButton* rename_button_{};
    QPushButton* duplicate_button_{};
    QPushButton* delete_button_{};
    document::Document document_;
    document::LayerId active_layer_id_{};
};

} // namespace rosettelab::ui
