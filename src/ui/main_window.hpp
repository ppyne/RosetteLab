#pragma once

#include "rosettelab/curves/polar_rose.hpp"
#include "rosettelab/document/document.hpp"

#include <QColor>
#include <QMainWindow>
#include <QString>

#include <cstddef>
#include <limits>
#include <vector>

class QAction;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QMenu;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QSplitter;
class QSpinBox;

namespace rosettelab::ui {

class ColorPreviewButton;
class PreviewWidget;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void add_polar_rose();
    void add_ellipse();
    void add_trochoid(document::CurveType type);
    void add_lissajous();
    void add_harmonograph();
    void new_document();
    void open_file();
    void open_document(const QString& path);
    void save();
    void save_as();
    bool save_document(const QString& path);
    void add_recent_file(const QString& path);
    void refresh_recent_files_menu();
    void clean_recent_files();
    bool confirm_discard_changes();
    void mark_document_modified();
    void set_document_modified(bool modified);
    void update_window_title();
    void reset_history();
    void undo();
    void redo();
    void restore_history_entry(std::size_t index);
    void update_history_actions();
    void export_raster(bool jpeg);
    void export_pdf();
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
    void load_saved_document_defaults();
    void save_document_defaults();
    void reset_document_defaults();
    void fit_to_workspace();
    void apply_zoom_level();
    void synchronize_zoom_level();
    void set_zoom_to_actual_size();
    void step_zoom_level(int direction, const QPoint& anchor);
    void pan_preview(const QPoint& movement);
    void refresh_k_mode_controls();
    void refresh_ellipse_radius_controls();
    void refresh_trochoid_trace_controls();
    void refresh_preset_choices();
    void apply_selected_preset();
    void restore_active_preset();
    void refresh_color_buttons();
    void refresh_layer_actions();
    void refresh_layer_preview(document::LayerId id);
    void refresh_all_layer_previews();
    void sync_layer_order();
    void update_preview();

    PreviewWidget* preview_{};
    QSplitter* main_splitter_{};
    QAction* save_action_{};
    QAction* undo_action_{};
    QAction* redo_action_{};
    QMenu* recent_files_menu_{};
    QDoubleSpinBox* page_width_{};
    QDoubleSpinBox* page_height_{};
    ColorPreviewButton* page_background_button_{};
    QPushButton* reset_document_defaults_button_{};
    QColor page_background_{Qt::white};
    QLabel* curve_type_label_{};
    QComboBox* preset_{};
    QPushButton* restore_preset_button_{};
    QDoubleSpinBox* radius_{};
    QComboBox* k_mode_{};
    QDoubleSpinBox* k_{};
    QSpinBox* numerator_{};
    QSpinBox* denominator_{};
    QDoubleSpinBox* phase_{};
    QDoubleSpinBox* rotation_{};
    QDoubleSpinBox* tolerance_{};
    QDoubleSpinBox* zoom_{};
    QComboBox* zoom_levels_{};
    QPushButton* fit_workspace_button_{};
    QPushButton* actual_size_button_{};
    QScrollArea* preview_scroll_{};
    bool applying_zoom_{false};
    QGroupBox* curve_group_{};
    QGroupBox* ellipse_group_{};
    QDoubleSpinBox* ellipse_radius_x_{};
    QDoubleSpinBox* ellipse_radius_y_{};
    QCheckBox* ellipse_link_radii_{};
    QDoubleSpinBox* ellipse_rotation_{};
    QDoubleSpinBox* ellipse_tolerance_{};
    QGroupBox* trochoid_group_{};
    QDoubleSpinBox* trochoid_fixed_radius_{};
    QDoubleSpinBox* trochoid_rolling_radius_{};
    QDoubleSpinBox* trochoid_pen_offset_{};
    QDoubleSpinBox* trochoid_rotation_{};
    QComboBox* trochoid_trace_mode_{};
    QDoubleSpinBox* trochoid_turns_{};
    QCheckBox* trochoid_close_limited_{};
    QDoubleSpinBox* trochoid_tolerance_{};
    QGroupBox* lissajous_group_{};
    QDoubleSpinBox* lissajous_amplitude_x_{};
    QDoubleSpinBox* lissajous_amplitude_y_{};
    QSpinBox* lissajous_frequency_x_{};
    QSpinBox* lissajous_frequency_y_{};
    QDoubleSpinBox* lissajous_phase_x_{};
    QDoubleSpinBox* lissajous_phase_y_{};
    QDoubleSpinBox* lissajous_rotation_{};
    QDoubleSpinBox* lissajous_tolerance_{};
    QGroupBox* harmonograph_group_{};
    QDoubleSpinBox* harmonograph_amplitude_x_{};
    QDoubleSpinBox* harmonograph_amplitude_y_{};
    QDoubleSpinBox* harmonograph_frequency_x_{};
    QDoubleSpinBox* harmonograph_frequency_y_{};
    QDoubleSpinBox* harmonograph_phase_x_{};
    QDoubleSpinBox* harmonograph_phase_y_{};
    QDoubleSpinBox* harmonograph_damping_x_{};
    QDoubleSpinBox* harmonograph_damping_y_{};
    QDoubleSpinBox* harmonograph_duration_{};
    QDoubleSpinBox* harmonograph_rotation_{};
    QDoubleSpinBox* harmonograph_tolerance_{};
    QGroupBox* appearance_group_{};
    QCheckBox* stroke_enabled_{};
    ColorPreviewButton* stroke_color_button_{};
    ColorPreviewButton* fill_color_button_{};
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
    QString current_file_path_;
    bool document_modified_{false};
    bool track_document_changes_{false};
    bool applying_preset_{false};
    QString active_preset_id_;
    struct HistoryEntry {
        document::Document document;
        document::LayerId active_layer_id{};
    };
    std::vector<HistoryEntry> history_;
    std::size_t history_index_{0};
    std::size_t saved_history_index_{0};
    static constexpr std::size_t no_history_index = std::numeric_limits<std::size_t>::max();
};

} // namespace rosettelab::ui
