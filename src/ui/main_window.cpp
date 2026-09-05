#include "ui/main_window.hpp"

#include "ui/color_editor_dialog.hpp"
#include "ui/color_preview_button.hpp"
#include "ui/layer_list_item_widget.hpp"
#include "ui/preview_widget.hpp"

#include "render/document_renderer.hpp"
#include "rosettelab/pdf/pdf_serializer.hpp"
#include "rosettelab/svg/svg_serializer.hpp"
#include "svg/qt_svg_parser.hpp"

#include <QAbstractItemModel>
#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QPdfOutputIntent>
#endif
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QResizeEvent>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <exception>
#include <initializer_list>
#include <utility>

namespace rosettelab::ui {
namespace {

constexpr auto open_directory_setting = "paths/openDirectory";
constexpr auto save_as_directory_setting = "paths/saveAsDirectory";
constexpr auto export_directory_setting = "paths/exportDirectory";

QString remembered_directory(const char* setting)
{
    const auto directory = QSettings{}.value(setting).toString();
    return directory.isEmpty() ? QDir::homePath() : directory;
}

void remember_selected_directory(const char* setting, const QString& path)
{
    QSettings{}.setValue(setting, QFileInfo(path).absolutePath());
}

QString suggested_output_path(
    const char* directory_setting,
    const QString& current_file_path,
    const QString& extension)
{
    const QString base_name = current_file_path.isEmpty()
        ? QStringLiteral("Untitled")
        : QFileInfo(current_file_path).completeBaseName();
    return QDir(remembered_directory(directory_setting))
        .filePath(base_name + QLatin1Char('.') + extension);
}

QDoubleSpinBox* angle_control(QWidget* parent)
{
    auto* control = new QDoubleSpinBox(parent);
    control->setRange(-3600.0, 3600.0);
    control->setDecimals(2);
    control->setSuffix(" deg");
    return control;
}

document::RgbaColor rgba_from_qcolor(const QColor& color)
{
    return {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
}

QColor qcolor_from_rgba(const document::RgbaColor& color)
{
    return QColor::fromRgbF(color.red, color.green, color.blue, color.alpha);
}

void style_color_button(ColorPreviewButton* button, const QColor& color)
{
    button->setText(QStringLiteral("#%1%2%3%4")
        .arg(color.red(), 2, 16, QLatin1Char('0'))
        .arg(color.green(), 2, 16, QLatin1Char('0'))
        .arg(color.blue(), 2, 16, QLatin1Char('0'))
        .arg(color.alpha(), 2, 16, QLatin1Char('0'))
        .toUpper());
    button->set_preview_color(color);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("RosetteLab");
    resize(1200, 760);

    auto* file_menu = menuBar()->addMenu("&File");
    auto* new_action = file_menu->addAction("New");
    new_action->setShortcut(QKeySequence::New);
    connect(new_action, &QAction::triggered, this, [this] { new_document(); });
    auto* open_action = file_menu->addAction("Open...");
    open_action->setShortcut(QKeySequence::Open);
    connect(open_action, &QAction::triggered, this, [this] { open_file(); });
    recent_files_menu_ = file_menu->addMenu("Open Recent");
    refresh_recent_files_menu();
    file_menu->addSeparator();
    save_action_ = file_menu->addAction("Save");
    save_action_->setShortcut(QKeySequence::Save);
    save_action_->setEnabled(false);
    connect(save_action_, &QAction::triggered, this, [this] { save(); });
    auto* save_as_action = file_menu->addAction("Save As...");
    save_as_action->setShortcut(QKeySequence::SaveAs);
    connect(save_as_action, &QAction::triggered, this, [this] { save_as(); });
    file_menu->addSeparator();
    auto* export_menu = file_menu->addMenu("Export");
    export_menu->addAction("To SVG...", this, [this] { export_svg(); });
    export_menu->addAction("To PNG...", this, [this] { export_raster(false); });
    export_menu->addAction("To JPEG...", this, [this] { export_raster(true); });
    export_menu->addAction("To PDF...", this, [this] { export_pdf(); });

    auto* edit_menu = menuBar()->addMenu("&Edit");
    undo_action_ = edit_menu->addAction("Undo");
    undo_action_->setObjectName("undoAction");
    undo_action_->setShortcut(QKeySequence::Undo);
    connect(undo_action_, &QAction::triggered, this, [this] { undo(); });
    redo_action_ = edit_menu->addAction("Redo");
    redo_action_->setObjectName("redoAction");
    redo_action_->setShortcut(QKeySequence::Redo);
    connect(redo_action_, &QAction::triggered, this, [this] { redo(); });

    main_splitter_ = new QSplitter(Qt::Horizontal, this);
    main_splitter_->setObjectName("mainSplitter");
    setCentralWidget(main_splitter_);

    auto* parameters_scroll = new QScrollArea(main_splitter_);
    parameters_scroll->setWidgetResizable(true);
    parameters_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    parameters_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    parameters_scroll->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    parameters_scroll->setMinimumWidth(260);
    auto* parameters_panel = new QWidget;
    parameters_scroll->setWidget(parameters_panel);
    auto* parameters_layout = new QVBoxLayout(parameters_panel);
    curve_type_label_ = new QLabel("Polar rose", parameters_panel);
    parameters_layout->addWidget(curve_type_label_);

    auto* document_group = new QGroupBox("Document", parameters_panel);
    auto* document_form = new QFormLayout(document_group);
    page_width_ = new QDoubleSpinBox(document_group);
    page_width_->setRange(1.0, 100000.0);
    page_width_->setValue(210.0);
    page_width_->setDecimals(2);
    page_width_->setSuffix(" mm");
    page_height_ = new QDoubleSpinBox(document_group);
    page_height_->setRange(1.0, 100000.0);
    page_height_->setValue(210.0);
    page_height_->setDecimals(2);
    page_height_->setSuffix(" mm");
    page_background_button_ = new ColorPreviewButton(document_group);
    reset_document_defaults_button_ = new QPushButton("Reset defaults", document_group);
    document_form->addRow("Page width", page_width_);
    document_form->addRow("Page height", page_height_);
    document_form->addRow("Background", page_background_button_);
    document_form->addRow("", reset_document_defaults_button_);
    parameters_layout->addWidget(document_group);
    style_color_button(page_background_button_, page_background_);

    auto* preset_group = new QGroupBox("Preset", parameters_panel);
    auto* preset_layout = new QHBoxLayout(preset_group);
    preset_ = new QComboBox(preset_group);
    preset_->setObjectName("presetSelector");
    restore_preset_button_ = new QPushButton("Restore preset", preset_group);
    restore_preset_button_->setObjectName("restorePresetButton");
    restore_preset_button_->setEnabled(false);
    preset_layout->addWidget(preset_, 1);
    preset_layout->addWidget(restore_preset_button_);
    parameters_layout->addWidget(preset_group);

    curve_group_ = new QGroupBox("Curve parameters", parameters_panel);
    auto* form = new QFormLayout(curve_group_);

    radius_ = new QDoubleSpinBox(curve_group_);
    radius_->setRange(0.01, 100000.0);
    radius_->setValue(100.0);
    radius_->setDecimals(2);

    k_mode_ = new QComboBox(curve_group_);
    k_mode_->setObjectName("polarKModeSelector");
    k_mode_->addItem("Decimal", static_cast<int>(curves::PolarKMode::Decimal));
    k_mode_->addItem("Fraction", static_cast<int>(curves::PolarKMode::Fraction));

    k_ = new QDoubleSpinBox(curve_group_);
    k_->setObjectName("polarKField");
    k_->setRange(-1000.0, 1000.0);
    k_->setValue(7.0);
    k_->setDecimals(3);

    numerator_ = new QSpinBox(curve_group_);
    numerator_->setObjectName("polarNumeratorField");
    numerator_->setRange(1, 10000);
    numerator_->setValue(7);
    denominator_ = new QSpinBox(curve_group_);
    denominator_->setObjectName("polarDenominatorField");
    denominator_->setRange(1, 10000);
    denominator_->setValue(1);

    phase_ = angle_control(curve_group_);
    rotation_ = angle_control(curve_group_);

    tolerance_ = new QDoubleSpinBox(curve_group_);
    tolerance_->setRange(0.001, 10.0);
    tolerance_->setValue(0.05);
    tolerance_->setDecimals(3);
    tolerance_->setSingleStep(0.01);
    tolerance_->setSuffix(" units");
    tolerance_->setToolTip("Maximum geometric deviation from the mathematical curve; smaller values are more precise.");

    form->addRow("Radius a", radius_);
    form->addRow("k representation", k_mode_);
    form->addRow("Parameter k", k_);
    form->addRow("Numerator n", numerator_);
    form->addRow("Denominator d", denominator_);
    form->addRow("Phase", phase_);
    form->addRow("Rotation", rotation_);
    form->addRow("Curve tolerance", tolerance_);
    parameters_layout->addWidget(curve_group_);
    refresh_k_mode_controls();

    ellipse_group_ = new QGroupBox("Ellipse parameters", parameters_panel);
    auto* ellipse_form = new QFormLayout(ellipse_group_);
    ellipse_radius_x_ = new QDoubleSpinBox(ellipse_group_);
    ellipse_radius_x_->setRange(0.01, 100000.0);
    ellipse_radius_x_->setValue(80.0);
    ellipse_radius_x_->setDecimals(2);
    ellipse_radius_y_ = new QDoubleSpinBox(ellipse_group_);
    ellipse_radius_y_->setRange(0.01, 100000.0);
    ellipse_radius_y_->setValue(50.0);
    ellipse_radius_y_->setDecimals(2);
    ellipse_link_radii_ = new QCheckBox("Perfect circle — link radii", ellipse_group_);
    ellipse_rotation_ = angle_control(ellipse_group_);
    ellipse_tolerance_ = new QDoubleSpinBox(ellipse_group_);
    ellipse_tolerance_->setRange(0.001, 10.0);
    ellipse_tolerance_->setValue(0.05);
    ellipse_tolerance_->setDecimals(3);
    ellipse_tolerance_->setSingleStep(0.01);
    ellipse_tolerance_->setSuffix(" units");
    ellipse_tolerance_->setToolTip(
        "Maximum geometric deviation from the mathematical ellipse; smaller values are more precise.");
    ellipse_form->addRow("Constraint", ellipse_link_radii_);
    ellipse_form->addRow("Horizontal radius", ellipse_radius_x_);
    ellipse_form->addRow("Vertical radius", ellipse_radius_y_);
    ellipse_form->addRow("Rotation", ellipse_rotation_);
    ellipse_form->addRow("Curve tolerance", ellipse_tolerance_);
    ellipse_group_->hide();
    parameters_layout->addWidget(ellipse_group_);

    trochoid_group_ = new QGroupBox("Trochoid parameters", parameters_panel);
    auto* trochoid_form = new QFormLayout(trochoid_group_);
    trochoid_fixed_radius_ = new QDoubleSpinBox(trochoid_group_);
    trochoid_fixed_radius_->setRange(0.01, 100000.0);
    trochoid_fixed_radius_->setValue(105.0);
    trochoid_fixed_radius_->setDecimals(3);
    trochoid_rolling_radius_ = new QDoubleSpinBox(trochoid_group_);
    trochoid_rolling_radius_->setRange(0.01, 100000.0);
    trochoid_rolling_radius_->setValue(45.0);
    trochoid_rolling_radius_->setDecimals(3);
    trochoid_pen_offset_ = new QDoubleSpinBox(trochoid_group_);
    trochoid_pen_offset_->setRange(0.0, 100000.0);
    trochoid_pen_offset_->setValue(30.0);
    trochoid_pen_offset_->setDecimals(3);
    trochoid_rotation_ = angle_control(trochoid_group_);
    trochoid_trace_mode_ = new QComboBox(trochoid_group_);
    trochoid_trace_mode_->addItem("Complete closed curve", static_cast<int>(curves::TraceMode::Complete));
    trochoid_trace_mode_->addItem("Limited number of turns", static_cast<int>(curves::TraceMode::Limited));
    trochoid_turns_ = new QDoubleSpinBox(trochoid_group_);
    trochoid_turns_->setRange(0.01, 10000.0);
    trochoid_turns_->setValue(2.0);
    trochoid_turns_->setDecimals(3);
    trochoid_close_limited_ = new QCheckBox("Close with a straight segment", trochoid_group_);
    trochoid_tolerance_ = new QDoubleSpinBox(trochoid_group_);
    trochoid_tolerance_->setRange(0.001, 10.0);
    trochoid_tolerance_->setValue(0.05);
    trochoid_tolerance_->setDecimals(3);
    trochoid_tolerance_->setSingleStep(0.01);
    trochoid_tolerance_->setSuffix(" units");
    trochoid_form->addRow("Fixed radius / teeth R", trochoid_fixed_radius_);
    trochoid_form->addRow("Rolling radius / teeth r", trochoid_rolling_radius_);
    trochoid_form->addRow("Pen offset d", trochoid_pen_offset_);
    trochoid_form->addRow("Rotation", trochoid_rotation_);
    trochoid_form->addRow("Tracing mode", trochoid_trace_mode_);
    trochoid_form->addRow("Turns around fixed gear", trochoid_turns_);
    trochoid_form->addRow("Limited path closure", trochoid_close_limited_);
    trochoid_form->addRow("Curve tolerance", trochoid_tolerance_);
    trochoid_group_->hide();
    parameters_layout->addWidget(trochoid_group_);
    refresh_trochoid_trace_controls();

    lissajous_group_ = new QGroupBox("Lissajous parameters", parameters_panel);
    auto* lissajous_form = new QFormLayout(lissajous_group_);
    lissajous_amplitude_x_ = new QDoubleSpinBox(lissajous_group_);
    lissajous_amplitude_y_ = new QDoubleSpinBox(lissajous_group_);
    for (auto* control : {lissajous_amplitude_x_, lissajous_amplitude_y_}) {
        control->setRange(0.01, 100000.0); control->setDecimals(2); control->setValue(80.0);
    }
    lissajous_frequency_x_ = new QSpinBox(lissajous_group_);
    lissajous_frequency_y_ = new QSpinBox(lissajous_group_);
    for (auto* control : {lissajous_frequency_x_, lissajous_frequency_y_}) control->setRange(1, 1000);
    lissajous_frequency_x_->setValue(3); lissajous_frequency_y_->setValue(2);
    lissajous_phase_x_ = angle_control(lissajous_group_);
    lissajous_phase_y_ = angle_control(lissajous_group_);
    lissajous_phase_x_->setValue(90.0);
    lissajous_rotation_ = angle_control(lissajous_group_);
    lissajous_tolerance_ = new QDoubleSpinBox(lissajous_group_);
    lissajous_tolerance_->setRange(0.001, 10.0); lissajous_tolerance_->setDecimals(3);
    lissajous_tolerance_->setValue(0.05); lissajous_tolerance_->setSuffix(" units");
    lissajous_form->addRow("Amplitude X", lissajous_amplitude_x_);
    lissajous_form->addRow("Amplitude Y", lissajous_amplitude_y_);
    lissajous_form->addRow("Frequency X", lissajous_frequency_x_);
    lissajous_form->addRow("Frequency Y", lissajous_frequency_y_);
    lissajous_form->addRow("Phase X", lissajous_phase_x_);
    lissajous_form->addRow("Phase Y", lissajous_phase_y_);
    lissajous_form->addRow("Rotation", lissajous_rotation_);
    lissajous_form->addRow("Curve tolerance", lissajous_tolerance_);
    lissajous_group_->hide();
    parameters_layout->addWidget(lissajous_group_);

    harmonograph_group_ = new QGroupBox("Harmonograph parameters", parameters_panel);
    auto* harmonograph_form = new QFormLayout(harmonograph_group_);
    harmonograph_amplitude_x_=new QDoubleSpinBox(harmonograph_group_);
    harmonograph_amplitude_y_=new QDoubleSpinBox(harmonograph_group_);
    for (auto* c : {harmonograph_amplitude_x_,harmonograph_amplitude_y_}) { c->setRange(0.01,100000.0); c->setValue(80.0); }
    harmonograph_frequency_x_=new QDoubleSpinBox(harmonograph_group_);
    harmonograph_frequency_y_=new QDoubleSpinBox(harmonograph_group_);
    for (auto* c : {harmonograph_frequency_x_,harmonograph_frequency_y_}) { c->setRange(0.001,1000.0); c->setDecimals(4); }
    harmonograph_frequency_x_->setValue(3.0); harmonograph_frequency_y_->setValue(2.0);
    harmonograph_phase_x_=angle_control(harmonograph_group_); harmonograph_phase_x_->setValue(90.0);
    harmonograph_phase_y_=angle_control(harmonograph_group_);
    harmonograph_damping_x_=new QDoubleSpinBox(harmonograph_group_);
    harmonograph_damping_y_=new QDoubleSpinBox(harmonograph_group_);
    for (auto* c : {harmonograph_damping_x_,harmonograph_damping_y_}) { c->setRange(0.0,10.0); c->setDecimals(4); c->setSingleStep(0.001); }
    harmonograph_damping_x_->setValue(0.015); harmonograph_damping_y_->setValue(0.010);
    harmonograph_duration_=new QDoubleSpinBox(harmonograph_group_); harmonograph_duration_->setRange(0.01,10000.0); harmonograph_duration_->setValue(40.0);
    harmonograph_rotation_=angle_control(harmonograph_group_);
    harmonograph_tolerance_=new QDoubleSpinBox(harmonograph_group_); harmonograph_tolerance_->setRange(0.001,10.0); harmonograph_tolerance_->setDecimals(3); harmonograph_tolerance_->setValue(0.05); harmonograph_tolerance_->setSuffix(" units");
    harmonograph_form->addRow("Amplitude X",harmonograph_amplitude_x_); harmonograph_form->addRow("Amplitude Y",harmonograph_amplitude_y_);
    harmonograph_form->addRow("Frequency X",harmonograph_frequency_x_); harmonograph_form->addRow("Frequency Y",harmonograph_frequency_y_);
    harmonograph_form->addRow("Phase X",harmonograph_phase_x_); harmonograph_form->addRow("Phase Y",harmonograph_phase_y_);
    harmonograph_form->addRow("Damping X",harmonograph_damping_x_); harmonograph_form->addRow("Damping Y",harmonograph_damping_y_);
    harmonograph_form->addRow("Duration",harmonograph_duration_); harmonograph_form->addRow("Rotation",harmonograph_rotation_);
    harmonograph_form->addRow("Curve tolerance",harmonograph_tolerance_);
    harmonograph_group_->hide(); parameters_layout->addWidget(harmonograph_group_);

    droplet_group_ = new QGroupBox("Droplet Rosette parameters", parameters_panel);
    auto* droplet_form = new QFormLayout(droplet_group_);
    droplet_count_ = new QSpinBox(droplet_group_);
    droplet_count_->setRange(2, 128); droplet_count_->setValue(3);
    droplet_outer_radius_ = new QDoubleSpinBox(droplet_group_);
    droplet_core_radius_ = new QDoubleSpinBox(droplet_group_);
    for (auto* control : {droplet_outer_radius_, droplet_core_radius_}) {
        control->setRange(0.0, 100000.0); control->setDecimals(2); control->setSuffix(" units");
    }
    droplet_outer_radius_->setMinimum(0.01); droplet_outer_radius_->setValue(80.0);
    droplet_core_radius_->setValue(16.0);
    droplet_swirl_ = angle_control(droplet_group_); droplet_swirl_->setValue(28.0);
    droplet_width_ = new QDoubleSpinBox(droplet_group_);
    droplet_width_->setRange(1.0, 100.0); droplet_width_->setValue(88.0); droplet_width_->setSuffix(" %");
    droplet_roundness_ = new QDoubleSpinBox(droplet_group_);
    droplet_roundness_->setRange(0.05, 1.0); droplet_roundness_->setValue(0.55);
    droplet_roundness_->setDecimals(2); droplet_roundness_->setSingleStep(0.05);
    droplet_rotation_ = angle_control(droplet_group_); droplet_rotation_->setValue(-90.0);
    droplet_form->addRow("Droplets", droplet_count_);
    droplet_form->addRow("Outer radius", droplet_outer_radius_);
    droplet_form->addRow("Core radius", droplet_core_radius_);
    droplet_form->addRow("Swirl", droplet_swirl_);
    droplet_form->addRow("Angular width", droplet_width_);
    droplet_form->addRow("Roundness", droplet_roundness_);
    droplet_form->addRow("Rotation", droplet_rotation_);
    droplet_group_->hide(); parameters_layout->addWidget(droplet_group_);

    transform_group_ = new QGroupBox("Layer transform", parameters_panel);
    auto* transform_form = new QFormLayout(transform_group_);
    transform_x_ = new QDoubleSpinBox(transform_group_);
    transform_x_->setObjectName("transformXField");
    transform_y_ = new QDoubleSpinBox(transform_group_);
    transform_y_->setObjectName("transformYField");
    for (auto* control : {transform_x_, transform_y_}) {
        control->setRange(-100000.0, 100000.0);
        control->setDecimals(3);
        control->setSuffix(" units");
    }
    transform_scale_x_ = new QDoubleSpinBox(transform_group_);
    transform_scale_y_ = new QDoubleSpinBox(transform_group_);
    for (auto* control : {transform_scale_x_, transform_scale_y_}) {
        control->setRange(0.01, 100000.0);
        control->setDecimals(2);
        control->setValue(100.0);
        control->setSuffix(" %");
    }
    transform_link_scales_ = new QCheckBox("Link X/Y scales", transform_group_);
    transform_link_scales_->setChecked(true);
    transform_rotation_ = angle_control(transform_group_);
    reset_transform_button_ = new QPushButton("Reset transform", transform_group_);
    reset_transform_button_->setObjectName("resetTransformButton");
    transform_form->addRow("Position X", transform_x_);
    transform_form->addRow("Position Y", transform_y_);
    transform_form->addRow("Scale X", transform_scale_x_);
    transform_form->addRow("Scale Y", transform_scale_y_);
    transform_form->addRow("", transform_link_scales_);
    transform_form->addRow("Rotation", transform_rotation_);
    transform_form->addRow("", reset_transform_button_);

    copies_group_ = new QGroupBox("Copies", parameters_panel);
    auto* copies_form = new QFormLayout(copies_group_);
    copy_arrangement_ = new QComboBox(copies_group_);
    copy_arrangement_->setObjectName("copyArrangementSelector");
    copy_arrangement_->addItem("Superimposed", static_cast<int>(document::CopyArrangement::Superimposed));
    copy_arrangement_->addItem("Linear", static_cast<int>(document::CopyArrangement::Linear));
    copy_arrangement_->addItem("Circular", static_cast<int>(document::CopyArrangement::Circular));
    copy_count_ = new QSpinBox(copies_group_);
    copy_count_->setObjectName("copyCountField");
    copy_count_->setRange(1, 1000);
    copy_rotation_ = angle_control(copies_group_);
    copy_scale_ = new QDoubleSpinBox(copies_group_);
    copy_scale_->setRange(0.01, 1000.0);
    copy_scale_->setDecimals(2);
    copy_scale_->setValue(100.0);
    copy_scale_->setSuffix(" %");
    copy_offset_x_ = new QDoubleSpinBox(copies_group_);
    copy_offset_y_ = new QDoubleSpinBox(copies_group_);
    for (auto* control : {copy_offset_x_, copy_offset_y_}) {
        control->setRange(-100000.0, 100000.0);
        control->setDecimals(3);
        control->setSuffix(" units");
    }
    copy_circular_radius_ = new QDoubleSpinBox(copies_group_);
    copy_circular_radius_->setObjectName("copyCircularRadiusField");
    copy_circular_radius_->setRange(0.0, 100000.0);
    copy_circular_radius_->setDecimals(3);
    copy_circular_radius_->setSuffix(" units");
    copy_circular_start_ = angle_control(copies_group_);
    copy_circular_angle_ = angle_control(copies_group_);
    copy_circular_angle_->setObjectName("copyCircularAngleField");
    copy_rotate_with_orbit_ = new QCheckBox("Enabled", copies_group_);
    copy_rotate_with_orbit_->setChecked(true);
    distribute_copies_button_ = new QPushButton("Distribute over 360 deg", copies_group_);
    distribute_copies_button_->setObjectName("distributeCopiesButton");
    copies_form->addRow("Arrangement", copy_arrangement_);
    copies_form->addRow("Count", copy_count_);
    copies_form->addRow("Rotation per copy", copy_rotation_);
    copies_form->addRow("Scale per copy", copy_scale_);
    copies_form->addRow("Offset X per copy", copy_offset_x_);
    copies_form->addRow("Offset Y per copy", copy_offset_y_);
    copies_form->addRow("Circular radius", copy_circular_radius_);
    copies_form->addRow("Start angle", copy_circular_start_);
    copies_form->addRow("Angle per copy", copy_circular_angle_);
    copies_form->addRow("Rotate with orbit", copy_rotate_with_orbit_);
    copies_form->addRow("", distribute_copies_button_);
    reset_copies_button_ = new QPushButton("Reset copies", copies_group_);
    reset_copies_button_->setObjectName("resetCopiesButton");
    copies_form->addRow("", reset_copies_button_);

    appearance_group_ = new QGroupBox("Appearance", parameters_panel);
    auto* appearance_form = new QFormLayout(appearance_group_);
    stroke_enabled_ = new QCheckBox("Enabled", appearance_group_);
    stroke_enabled_->setChecked(true);
    stroke_color_button_ = new ColorPreviewButton(appearance_group_);
    fill_color_button_ = new ColorPreviewButton(appearance_group_);

    stroke_width_ = new QDoubleSpinBox(appearance_group_);
    stroke_width_->setRange(0.0, 1000.0);
    stroke_width_->setValue(0.6);
    stroke_width_->setDecimals(2);
    stroke_width_->setSuffix(" units");

    fill_enabled_ = new QCheckBox("Enabled", appearance_group_);
    fill_rule_ = new QComboBox(appearance_group_);
    fill_rule_->addItem("Non-zero", static_cast<int>(document::FillRule::NonZero));
    fill_rule_->addItem("Even-odd", static_cast<int>(document::FillRule::EvenOdd));
    fill_rule_->setCurrentIndex(fill_rule_->findData(static_cast<int>(document::FillRule::EvenOdd)));

    layer_opacity_ = new QSpinBox(appearance_group_);
    layer_opacity_->setRange(0, 100);
    layer_opacity_->setValue(100);
    layer_opacity_->setSuffix(" %");

    blend_mode_ = new QComboBox(appearance_group_);
    blend_mode_->setObjectName("blendModeSelector");
    const std::initializer_list<std::pair<const char*, document::BlendMode>> blend_modes{
        {"Normal", document::BlendMode::Normal}, {"Multiply", document::BlendMode::Multiply},
        {"Screen", document::BlendMode::Screen}, {"Overlay", document::BlendMode::Overlay},
        {"Darken", document::BlendMode::Darken}, {"Lighten", document::BlendMode::Lighten},
        {"Color dodge", document::BlendMode::ColorDodge}, {"Color burn", document::BlendMode::ColorBurn},
        {"Hard light", document::BlendMode::HardLight}, {"Soft light", document::BlendMode::SoftLight},
        {"Difference", document::BlendMode::Difference}, {"Exclusion", document::BlendMode::Exclusion},
        {"Hue", document::BlendMode::Hue}, {"Saturation", document::BlendMode::Saturation},
        {"Color", document::BlendMode::Color}, {"Luminosity", document::BlendMode::Luminosity},
    };
    for (const auto& [name, mode] : blend_modes) {
        blend_mode_->addItem(name, static_cast<int>(mode));
        if (mode == document::BlendMode::Hue || mode == document::BlendMode::Saturation
            || mode == document::BlendMode::Color || mode == document::BlendMode::Luminosity) {
            const int index = blend_mode_->count() - 1;
            if (auto* model = qobject_cast<QStandardItemModel*>(blend_mode_->model())) {
                model->item(index)->setEnabled(false);
                model->item(index)->setToolTip(
                    "Unavailable: the Qt preview backend cannot render this blend mode accurately.");
            }
        }
    }

    appearance_form->addRow("Stroke", stroke_enabled_);
    appearance_form->addRow("Stroke color", stroke_color_button_);
    appearance_form->addRow("Stroke width", stroke_width_);
    appearance_form->addRow("Fill", fill_enabled_);
    appearance_form->addRow("Fill color", fill_color_button_);
    appearance_form->addRow("Fill rule", fill_rule_);
    appearance_form->addRow("Layer opacity", layer_opacity_);
    appearance_form->addRow("Blend mode", blend_mode_);
    parameters_layout->addWidget(appearance_group_);
    parameters_layout->addWidget(transform_group_);
    parameters_layout->addWidget(copies_group_);
    refresh_color_buttons();

    auto* view_group = new QGroupBox("View", parameters_panel);
    auto* view_form = new QFormLayout(view_group);
    zoom_ = new QDoubleSpinBox(view_group);
    zoom_->setObjectName("zoomField");
    zoom_->setRange(0.1, 3200.0);
    zoom_->setDecimals(2);
    zoom_->setValue(100.0);
    zoom_->setSuffix(" %");
    zoom_levels_ = new QComboBox(view_group);
    zoom_levels_->setObjectName("zoomLevelSelector");
    zoom_levels_->addItem("Fit to workspace", -1.0);
    zoom_levels_->addItem("Custom", 0.0);
    for (const double level : {0.10,0.20,0.30,0.40,0.50,0.75,1.00,1.50,2.00,3.00,4.00,5.00,
            6.25,8.33,12.50,16.67,25.00,33.33,50.00,66.67,100.00,125.00,150.00,200.00,
            300.00,400.00,500.00,600.00,800.00,1200.00,1600.00,3200.00}) {
        zoom_levels_->addItem(QStringLiteral("%1 %").arg(level,0,'f',2),level);
    }
    fit_workspace_button_ = new QPushButton("Fit to workspace", view_group);
    actual_size_button_ = new QPushButton("Actual size (100%)", view_group);
    view_form->addRow("Zoom", zoom_);
    view_form->addRow("Zoom level", zoom_levels_);
    view_form->addRow("", fit_workspace_button_);
    view_form->addRow("", actual_size_button_);
    parameters_layout->addWidget(view_group);
    parameters_layout->addStretch();

    preview_scroll_ = new QScrollArea(main_splitter_);
    preview_scroll_->setWidgetResizable(false);
    preview_scroll_->setAlignment(Qt::AlignCenter);
    preview_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    preview_scroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    preview_ = new PreviewWidget;
    preview_->set_document(&document_);
    preview_->set_wheel_zoom_handler(
        [this](const int direction,const QPoint& anchor){ step_zoom_level(direction,anchor); });
    preview_->set_pan_handler([this](const QPoint& movement){ pan_preview(movement); });
    preview_scroll_->setWidget(preview_);

    auto* layers_panel = new QWidget(main_splitter_);
    auto* layers_layout = new QVBoxLayout(layers_panel);
    layers_layout->addWidget(new QLabel("Layers", layers_panel));
    layers_ = new QListWidget(layers_panel);
    layers_->setDragDropMode(QAbstractItemView::InternalMove);
    auto& initial = document_.add_polar_rose();
    initial.preset_id = "rose-seven";
    load_saved_document_defaults();
    active_layer_id_ = initial.id;
    auto* initial_layer = add_layer_row(initial);
    layers_->setCurrentItem(initial_layer);
    layers_layout->addWidget(layers_);
    auto* add_button = new QPushButton("Add new layer...", layers_panel);
    auto* add_menu = new QMenu(add_button);
    add_menu->addAction("Polar rose", this, [this] { add_polar_rose(); });
    add_menu->addAction("Ellipse", this, [this] { add_ellipse(); });
    add_menu->addAction("Hypotrochoid", this, [this] {
        add_trochoid(document::CurveType::Hypotrochoid);
    });
    add_menu->addAction("Epitrochoid", this, [this] {
        add_trochoid(document::CurveType::Epitrochoid);
    });
    add_menu->addAction("Lissajous", this, [this] { add_lissajous(); });
    add_menu->addAction("Harmonograph", this, [this] { add_harmonograph(); });
    add_menu->addAction("Droplet Rosette", this, [this] { add_droplet_rosette(); });
    add_button->setMenu(add_menu);
    layers_layout->addWidget(add_button);

    auto* layer_actions = new QHBoxLayout;
    rename_button_ = new QPushButton("Rename", layers_panel);
    duplicate_button_ = new QPushButton("Duplicate", layers_panel);
    delete_button_ = new QPushButton("Delete", layers_panel);
    layer_actions->addWidget(rename_button_);
    layer_actions->addWidget(duplicate_button_);
    layer_actions->addWidget(delete_button_);
    layers_layout->addLayout(layer_actions);

    main_splitter_->setStretchFactor(0, 0);
    main_splitter_->setStretchFactor(1, 1);
    main_splitter_->setStretchFactor(2, 0);
    main_splitter_->setSizes({280, 640, 280});

    connect(radius_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.polar.radius"); });
    connect(page_width_, &QDoubleSpinBox::valueChanged, this, [this] { update_document_settings("document.width"); });
    connect(page_height_, &QDoubleSpinBox::valueChanged, this, [this] { update_document_settings("document.height"); });
    connect(page_background_button_, &QPushButton::clicked, this, [this] { choose_page_background(); });
    connect(reset_document_defaults_button_, &QPushButton::clicked, this, [this] { reset_document_defaults(); });
    connect(preset_, &QComboBox::currentIndexChanged, this, [this](const int index) {
        if (index > 0 && !applying_preset_) apply_selected_preset();
    });
    connect(restore_preset_button_, &QPushButton::clicked, this, [this] { restore_active_preset(); });
    connect(k_mode_, &QComboBox::currentIndexChanged, this, [this] {
        refresh_k_mode_controls();
        update_preview();
    });
    connect(k_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.polar.k"); });
    connect(numerator_, &QSpinBox::valueChanged, this, [this] { update_preview("curve.polar.numerator"); });
    connect(denominator_, &QSpinBox::valueChanged, this, [this] { update_preview("curve.polar.denominator"); });
    connect(phase_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.polar.phase"); });
    connect(rotation_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.polar.rotation"); });
    connect(tolerance_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.polar.tolerance"); });
    connect(ellipse_radius_x_, &QDoubleSpinBox::valueChanged, this, [this] {
        if (ellipse_link_radii_->isChecked()) {
            const QSignalBlocker blocker(ellipse_radius_y_);
            ellipse_radius_y_->setValue(ellipse_radius_x_->value());
        }
        update_preview("curve.ellipse.radius-x");
    });
    connect(ellipse_radius_y_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.ellipse.radius-y"); });
    connect(ellipse_link_radii_, &QCheckBox::toggled, this, [this](const bool linked) {
        if (linked) {
            const QSignalBlocker blocker(ellipse_radius_y_);
            ellipse_radius_y_->setValue(ellipse_radius_x_->value());
        }
        refresh_ellipse_radius_controls();
        update_preview();
    });
    connect(ellipse_rotation_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.ellipse.rotation"); });
    connect(ellipse_tolerance_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.ellipse.tolerance"); });
    connect(trochoid_fixed_radius_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.trochoid.fixed-radius"); });
    connect(trochoid_rolling_radius_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.trochoid.rolling-radius"); });
    connect(trochoid_pen_offset_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.trochoid.pen-offset"); });
    connect(trochoid_rotation_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.trochoid.rotation"); });
    connect(trochoid_trace_mode_, &QComboBox::currentIndexChanged, this, [this] {
        refresh_trochoid_trace_controls();
        update_preview();
    });
    connect(trochoid_turns_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.trochoid.turns"); });
    connect(trochoid_close_limited_, &QCheckBox::toggled, this, [this] { update_preview(); });
    connect(trochoid_tolerance_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.trochoid.tolerance"); });
    connect(lissajous_amplitude_x_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.lissajous.amplitude-x"); });
    connect(lissajous_amplitude_y_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.lissajous.amplitude-y"); });
    connect(lissajous_frequency_x_, &QSpinBox::valueChanged, this, [this] { update_preview("curve.lissajous.frequency-x"); });
    connect(lissajous_frequency_y_, &QSpinBox::valueChanged, this, [this] { update_preview("curve.lissajous.frequency-y"); });
    connect(lissajous_phase_x_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.lissajous.phase-x"); });
    connect(lissajous_phase_y_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.lissajous.phase-y"); });
    connect(lissajous_rotation_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.lissajous.rotation"); });
    connect(lissajous_tolerance_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview("curve.lissajous.tolerance"); });
    for (auto* control : {harmonograph_amplitude_x_,harmonograph_amplitude_y_,harmonograph_frequency_x_,harmonograph_frequency_y_,harmonograph_phase_x_,harmonograph_phase_y_,harmonograph_damping_x_,harmonograph_damping_y_,harmonograph_duration_,harmonograph_rotation_,harmonograph_tolerance_})
        connect(control,&QDoubleSpinBox::valueChanged,this,[this,control]{
            update_preview(QStringLiteral("curve.harmonograph.%1").arg(reinterpret_cast<quintptr>(control)));
        });
    connect(droplet_count_, &QSpinBox::valueChanged, this, [this] { update_preview("curve.droplet.count"); });
    for (auto* control : {droplet_outer_radius_, droplet_core_radius_, droplet_swirl_,
                          droplet_width_, droplet_roundness_, droplet_rotation_}) {
        connect(control, &QDoubleSpinBox::valueChanged, this, [this, control] {
            update_preview(QStringLiteral("curve.droplet.%1").arg(reinterpret_cast<quintptr>(control)));
        });
    }
    connect(transform_x_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("transform.x"); });
    connect(transform_y_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("transform.y"); });
    connect(transform_scale_x_, &QDoubleSpinBox::valueChanged, this, [this] {
        if (transform_link_scales_->isChecked()) {
            const QSignalBlocker blocker(transform_scale_y_);
            transform_scale_y_->setValue(transform_scale_x_->value());
        }
        update_layer_transform("transform.scale-x");
    });
    connect(transform_scale_y_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("transform.scale-y"); });
    connect(transform_link_scales_, &QCheckBox::toggled, this, [this](const bool linked) {
        if (linked) {
            const QSignalBlocker blocker(transform_scale_y_);
            transform_scale_y_->setValue(transform_scale_x_->value());
        }
        refresh_transform_controls();
        update_layer_transform();
    });
    connect(transform_rotation_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("transform.rotation"); });
    connect(reset_transform_button_, &QPushButton::clicked, this, [this] { reset_layer_transform(); });
    connect(copy_count_, &QSpinBox::valueChanged, this, [this] { update_layer_transform("copies.count"); });
    connect(copy_arrangement_, &QComboBox::currentIndexChanged, this, [this] {
        refresh_copy_controls();
        update_layer_transform();
    });
    connect(copy_rotation_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("copies.rotation"); });
    connect(copy_scale_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("copies.scale"); });
    connect(copy_offset_x_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("copies.offset-x"); });
    connect(copy_offset_y_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("copies.offset-y"); });
    connect(copy_circular_radius_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("copies.circular-radius"); });
    connect(copy_circular_start_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("copies.circular-start"); });
    connect(copy_circular_angle_, &QDoubleSpinBox::valueChanged, this, [this] { update_layer_transform("copies.circular-angle"); });
    connect(copy_rotate_with_orbit_, &QCheckBox::toggled, this, [this] { update_layer_transform(); });
    connect(distribute_copies_button_, &QPushButton::clicked, this, [this] { distribute_copies_over_circle(); });
    connect(reset_copies_button_, &QPushButton::clicked, this, [this] { reset_layer_copies(); });
    connect(stroke_color_button_, &QPushButton::clicked, this, [this] { choose_stroke_color(); });
    connect(fill_color_button_, &QPushButton::clicked, this, [this] { choose_fill_color(); });
    connect(stroke_enabled_, &QCheckBox::toggled, this, [this] { update_appearance(); });
    connect(stroke_width_, &QDoubleSpinBox::valueChanged, this, [this] { update_appearance("appearance.stroke-width"); });
    connect(fill_enabled_, &QCheckBox::toggled, this, [this] { update_appearance(); });
    connect(fill_rule_, &QComboBox::currentIndexChanged, this, [this] { update_appearance(); });
    connect(layer_opacity_, &QSpinBox::valueChanged, this, [this] { update_appearance("appearance.opacity"); });
    connect(blend_mode_, &QComboBox::currentIndexChanged, this, [this] { update_appearance(); });
    connect(zoom_, &QDoubleSpinBox::valueChanged, this, [this](const double value) {
        preview_->set_zoom_percent(value);
        if (!applying_zoom_) synchronize_zoom_level();
    });
    connect(zoom_levels_, &QComboBox::currentIndexChanged, this, [this] { apply_zoom_level(); });
    connect(fit_workspace_button_, &QPushButton::clicked, this, [this] {
        zoom_levels_->setCurrentIndex(0); fit_to_workspace();
    });
    connect(actual_size_button_, &QPushButton::clicked, this, [this] { set_zoom_to_actual_size(); });
    connect(main_splitter_, &QSplitter::splitterMoved, this, [this] {
        if (zoom_levels_->currentData().toDouble()<0.0) fit_to_workspace();
    });
    connect(add_button, &QPushButton::clicked, this, [this] { add_polar_rose(); });
    connect(rename_button_, &QPushButton::clicked, this, [this] { rename_active_layer(); });
    connect(duplicate_button_, &QPushButton::clicked, this, [this] { duplicate_active_layer(); });
    connect(delete_button_, &QPushButton::clicked, this, [this] { delete_active_layer(); });
    connect(layers_, &QListWidget::currentItemChanged, this,
        [this](QListWidgetItem* current, QListWidgetItem*) {
            if (current != nullptr) {
                select_layer(current->data(Qt::UserRole).toULongLong());
            }
        });
    connect(layers_->model(), &QAbstractItemModel::rowsMoved, this,
        [this] { sync_layer_order(); });

    // The initial row was selected before the layer-list signals were connected.
    // Explicitly run the same synchronization used by New and Open.
    load_active_layer();
    load_document_settings();
    preview_->update();
    refresh_layer_actions();

    QSettings settings;
    const auto geometry = settings.value("mainWindow/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    const auto splitter_state = settings.value("mainWindow/splitterState").toByteArray();
    if (!splitter_state.isEmpty()) {
        main_splitter_->restoreState(splitter_state);
    }
    track_document_changes_ = true;
    reset_history();
    QTimer::singleShot(0,this,[this]{ fit_to_workspace(); });
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!confirm_discard_changes()) {
        event->ignore();
        return;
    }
    QSettings settings;
    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/splitterState", main_splitter_->saveState());
    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (zoom_levels_!=nullptr && zoom_levels_->currentData().toDouble()<0.0)
        QTimer::singleShot(0,this,[this]{ fit_to_workspace(); });
}

void MainWindow::open_file()
{
    const auto path = QFileDialog::getOpenFileName(
        this, "Open RosetteLab SVG", remembered_directory(open_directory_setting),
        "RosetteLab SVG (*.svg)");
    if (path.isEmpty()) {
        return;
    }
    remember_selected_directory(open_directory_setting, path);

    open_document(path);
}

void MainWindow::new_document()
{
    if (!confirm_discard_changes()) {
        return;
    }
    document_ = document::Document{};
    load_saved_document_defaults();
    auto& initial = document_.add_polar_rose();
    initial.preset_id = "rose-seven";
    active_layer_id_ = initial.id;
    current_file_path_.clear();
    rebuild_layer_list();
    load_document_settings();
    zoom_levels_->setCurrentIndex(0);
    fit_to_workspace();
    preview_->update();
    save_action_->setEnabled(false);
    reset_history();
}

void MainWindow::open_document(const QString& path)
{
    if (!confirm_discard_changes()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Unable to open", file.errorString());
        return;
    }

    try {
        document_ = svg::parse_rosettelab_svg(file.readAll());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Unable to open", error.what());
        return;
    }
    rebuild_layer_list();
    load_document_settings();
    preview_->update();
    current_file_path_ = QFileInfo(path).absoluteFilePath();
    save_action_->setEnabled(true);
    add_recent_file(current_file_path_);
    reset_history();
}

void MainWindow::add_recent_file(const QString& path)
{
    QSettings settings;
    auto files = settings.value("files/recent").toStringList();
    const auto absolute_path = QFileInfo(path).absoluteFilePath();
    files.removeAll(absolute_path);
    files.prepend(absolute_path);
    while (files.size() > 6) {
        files.removeLast();
    }
    settings.setValue("files/recent", files);
    refresh_recent_files_menu();
}

void MainWindow::refresh_recent_files_menu()
{
    recent_files_menu_->clear();
    const auto files = QSettings{}.value("files/recent").toStringList();
    for (const auto& path : files) {
        auto* action = recent_files_menu_->addAction(QFileInfo(path).fileName());
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, [this, path] { open_document(path); });
    }
    if (!files.isEmpty()) {
        recent_files_menu_->addSeparator();
    }
    auto* clean_action = recent_files_menu_->addAction("Clean History");
    clean_action->setEnabled(!files.isEmpty());
    connect(clean_action, &QAction::triggered, this, [this] { clean_recent_files(); });
}

void MainWindow::clean_recent_files()
{
    QSettings{}.remove("files/recent");
    refresh_recent_files_menu();
}

bool MainWindow::confirm_discard_changes()
{
    if (!document_modified_) {
        return true;
    }
    const auto answer = QMessageBox::warning(
        this,
        "Unsaved changes",
        "The current document has unsaved changes.",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);
    if (answer == QMessageBox::Cancel) {
        return false;
    }
    if (answer == QMessageBox::Save) {
        save();
        return !document_modified_;
    }
    return true;
}

void MainWindow::mark_document_modified(const QString& coalescing_key)
{
    if (!track_document_changes_) {
        return;
    }
    const auto effective_key = coalescing_key.isEmpty()
        ? QString{}
        : QStringLiteral("%1:%2").arg(active_layer_id_).arg(coalescing_key);
    if (!history_.empty() && history_index_ < history_.size() &&
        history_[history_index_].document == document_ &&
        history_[history_index_].active_layer_id == active_layer_id_) {
        if (effective_key.isEmpty()) {
            history_coalescing_key_.clear();
            history_coalescing_timer_.invalidate();
        }
        return;
    }
    constexpr qint64 coalescing_interval_ms = 500;
    const bool replace_current_entry = !effective_key.isEmpty() &&
        history_coalescing_timer_.isValid() &&
        history_coalescing_timer_.elapsed() <= coalescing_interval_ms &&
        history_coalescing_key_ == effective_key &&
        history_index_ + 1 == history_.size() &&
        history_index_ != saved_history_index_;
    if (replace_current_entry) {
        history_[history_index_] = {document_, active_layer_id_};
        history_coalescing_timer_.restart();
        set_document_modified(history_index_ != saved_history_index_);
        update_history_actions();
        return;
    }
    if (history_index_ + 1 < history_.size()) {
        if (saved_history_index_ > history_index_) {
            saved_history_index_ = no_history_index;
        }
        history_.erase(history_.begin() + static_cast<std::ptrdiff_t>(history_index_ + 1),
                       history_.end());
    }
    history_.push_back({document_, active_layer_id_});
    history_index_ = history_.size() - 1;
    constexpr std::size_t maximum_history_entries = 200;
    if (history_.size() > maximum_history_entries) {
        history_.erase(history_.begin());
        --history_index_;
        if (saved_history_index_ == 0) {
            saved_history_index_ = no_history_index;
        } else if (saved_history_index_ != no_history_index) {
            --saved_history_index_;
        }
    }
    set_document_modified(history_index_ != saved_history_index_);
    update_history_actions();
    history_coalescing_key_ = effective_key;
    if (effective_key.isEmpty()) {
        history_coalescing_timer_.invalidate();
    } else {
        history_coalescing_timer_.restart();
    }
}

void MainWindow::set_document_modified(const bool modified)
{
    document_modified_ = modified;
    setWindowModified(modified);
    update_window_title();
}

void MainWindow::update_window_title()
{
    if (current_file_path_.isEmpty()) {
        setWindowTitle("RosetteLab[*]");
    } else {
        setWindowTitle(QStringLiteral("RosetteLab - %1[*]")
            .arg(QFileInfo(current_file_path_).fileName()));
    }
}

void MainWindow::reset_history()
{
    history_coalescing_key_.clear();
    history_coalescing_timer_.invalidate();
    history_.clear();
    history_.push_back({document_, active_layer_id_});
    history_index_ = 0;
    saved_history_index_ = 0;
    set_document_modified(false);
    update_history_actions();
}

void MainWindow::undo()
{
    if (history_index_ == 0 || history_.empty()) {
        return;
    }
    restore_history_entry(history_index_ - 1);
}

void MainWindow::redo()
{
    if (history_.empty() || history_index_ + 1 >= history_.size()) {
        return;
    }
    restore_history_entry(history_index_ + 1);
}

void MainWindow::restore_history_entry(const std::size_t index)
{
    history_coalescing_key_.clear();
    history_coalescing_timer_.invalidate();
    track_document_changes_ = false;
    document_ = history_[index].document;
    const auto selected_id = history_[index].active_layer_id;
    rebuild_layer_list();
    load_document_settings();
    if (document_.find_layer(selected_id) != nullptr) {
        select_layer_row(selected_id);
    }
    preview_->update();
    history_index_ = index;
    track_document_changes_ = true;
    set_document_modified(history_index_ != saved_history_index_);
    update_history_actions();
}

void MainWindow::update_history_actions()
{
    undo_action_->setEnabled(!history_.empty() && history_index_ > 0);
    redo_action_->setEnabled(!history_.empty() && history_index_ + 1 < history_.size());
}

void MainWindow::choose_page_background()
{
    ColorEditorDialog dialog(page_background_, "Page background", this);
    if (dialog.exec() == QDialog::Accepted) {
        page_background_ = dialog.color();
        style_color_button(page_background_button_, page_background_);
        update_document_settings();
    }
}

void MainWindow::update_document_settings(const QString& coalescing_key)
{
    document_.settings().page_width = page_width_->value();
    document_.settings().page_height = page_height_->value();
    document_.settings().unit = "mm";
    document_.settings().background = rgba_from_qcolor(page_background_);
    preview_->refresh_document_geometry();
    save_document_defaults();
    if (zoom_levels_->currentData().toDouble()<0.0) fit_to_workspace();
    refresh_all_layer_previews();
    mark_document_modified(coalescing_key);
}

void MainWindow::load_saved_document_defaults()
{
    QSettings settings;
    auto& document_settings=document_.settings();
    document_settings.page_width=settings.value("documentDefaults/pageWidth",210.0).toDouble();
    document_settings.page_height=settings.value("documentDefaults/pageHeight",210.0).toDouble();
    document_settings.unit="mm";
    const auto color=settings.value("documentDefaults/background",QColor(Qt::white)).value<QColor>();
    document_settings.background=rgba_from_qcolor(color.isValid()?color:QColor(Qt::white));
}

void MainWindow::save_document_defaults()
{
    QSettings settings;
    settings.setValue("documentDefaults/pageWidth",document_.settings().page_width);
    settings.setValue("documentDefaults/pageHeight",document_.settings().page_height);
    settings.setValue("documentDefaults/background",qcolor_from_rgba(document_.settings().background));
}

void MainWindow::reset_document_defaults()
{
    const QSignalBlocker width_blocker(page_width_),height_blocker(page_height_);
    page_width_->setValue(210.0);
    page_height_->setValue(210.0);
    page_background_=QColor(Qt::white);
    style_color_button(page_background_button_,page_background_);
    update_document_settings();
}

void MainWindow::fit_to_workspace()
{
    if (preview_==nullptr || preview_scroll_==nullptr) return;
    const double level=preview_->fit_zoom_percent(preview_scroll_->viewport()->size());
    applying_zoom_=true;
    zoom_->setValue(level);
    preview_->set_zoom_percent(level);
    applying_zoom_=false;
}

void MainWindow::apply_zoom_level()
{
    if (applying_zoom_) return;
    const double level=zoom_levels_->currentData().toDouble();
    if (level<0.0) { fit_to_workspace(); return; }
    if (level==0.0) return;
    applying_zoom_=true;
    zoom_->setValue(level);
    preview_->set_zoom_percent(level);
    applying_zoom_=false;
}

void MainWindow::synchronize_zoom_level()
{
    const double value=zoom_->value();
    int found=-1;
    for (int index=2;index<zoom_levels_->count();++index) {
        if (std::abs(zoom_levels_->itemData(index).toDouble()-value)<0.005) { found=index; break; }
    }
    const QSignalBlocker blocker(zoom_levels_);
    zoom_levels_->setCurrentIndex(found>=0?found:1);
}

void MainWindow::set_zoom_to_actual_size()
{
    const int index=zoom_levels_->findData(100.0);
    if (index>=0) zoom_levels_->setCurrentIndex(index);
}

void MainWindow::step_zoom_level(const int direction, const QPoint& anchor)
{
    if (direction==0) return;
    constexpr double margin=40.0;
    const QSize old_size=preview_->size();
    const QPoint viewport_anchor=preview_->mapTo(preview_scroll_->viewport(),anchor);
    const double normalized_x=(anchor.x()-old_size.width()/2.0)/std::max(1.0,old_size.width()-margin);
    const double normalized_y=(anchor.y()-old_size.height()/2.0)/std::max(1.0,old_size.height()-margin);
    const double current=zoom_->value();
    int nearest=2;
    double distance=std::numeric_limits<double>::max();
    for (int index=2;index<zoom_levels_->count();++index) {
        const double candidate=zoom_levels_->itemData(index).toDouble();
        const double candidate_distance=std::abs(candidate-current);
        if (candidate_distance<distance) { distance=candidate_distance; nearest=index; }
    }
    int target=nearest;
    const double nearest_value=zoom_levels_->itemData(nearest).toDouble();
    if (direction>0 && nearest_value<=current+0.005) ++target;
    else if (direction<0 && nearest_value>=current-0.005) --target;
    target=std::clamp(target,2,zoom_levels_->count()-1);
    zoom_levels_->setCurrentIndex(target);

    const QSize new_size=preview_->size();
    const QPoint new_anchor(
        qRound(new_size.width()/2.0+normalized_x*std::max(1.0,new_size.width()-margin)),
        qRound(new_size.height()/2.0+normalized_y*std::max(1.0,new_size.height()-margin)));
    const QPoint mapped=preview_->mapTo(preview_scroll_->viewport(),new_anchor);
    auto* horizontal=preview_scroll_->horizontalScrollBar();
    auto* vertical=preview_scroll_->verticalScrollBar();
    if (horizontal->maximum()>0)
        horizontal->setValue(horizontal->value()+mapped.x()-viewport_anchor.x());
    if (vertical->maximum()>0)
        vertical->setValue(vertical->value()+mapped.y()-viewport_anchor.y());
}

void MainWindow::pan_preview(const QPoint& movement)
{
    auto* horizontal=preview_scroll_->horizontalScrollBar();
    auto* vertical=preview_scroll_->verticalScrollBar();
    horizontal->setValue(horizontal->value()-movement.x());
    vertical->setValue(vertical->value()-movement.y());
}

void MainWindow::load_document_settings()
{
    const QSignalBlocker width_blocker(page_width_);
    const QSignalBlocker height_blocker(page_height_);
    page_width_->setValue(document_.settings().page_width);
    page_height_->setValue(document_.settings().page_height);
    page_background_ = qcolor_from_rgba(document_.settings().background);
    style_color_button(page_background_button_, page_background_);
    preview_->refresh_document_geometry();
    if (zoom_levels_!=nullptr && zoom_levels_->currentData().toDouble()<0.0) fit_to_workspace();
    refresh_all_layer_previews();
}

void MainWindow::rebuild_layer_list()
{
    layers_->clear();
    active_layer_id_ = 0;
    for (const auto& layer : document_.layers()) {
        add_layer_row(layer, 0);
    }
    if (layers_->count() > 0) {
        layers_->setCurrentRow(0);
    } else {
        refresh_layer_actions();
    }
}

void MainWindow::save_as()
{
    auto path = QFileDialog::getSaveFileName(
        this, "Save RosetteLab SVG",
        suggested_output_path(save_as_directory_setting, current_file_path_, "svg"),
        "RosetteLab SVG (*.svg)");
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(".svg", Qt::CaseInsensitive)) {
        path += ".svg";
    }
    remember_selected_directory(save_as_directory_setting, path);

    static_cast<void>(save_document(path));
}

void MainWindow::save()
{
    if (current_file_path_.isEmpty()) {
        save_as();
        return;
    }
    static_cast<void>(save_document(current_file_path_));
}

bool MainWindow::save_document(const QString& path)
{

    std::string svg_text;
    try {
        svg_text = svg::serialize_rosettelab_svg(document_);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Unable to save", error.what());
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Unable to save", file.errorString());
        return false;
    }
    const auto data = QByteArray::fromStdString(svg_text);
    if (file.write(data) != data.size()) {
        QMessageBox::critical(this, "Unable to save", file.errorString());
        return false;
    }
    current_file_path_ = QFileInfo(path).absoluteFilePath();
    save_action_->setEnabled(true);
    add_recent_file(current_file_path_);
    saved_history_index_ = history_index_;
    set_document_modified(false);
    update_history_actions();
    return true;
}

void MainWindow::export_svg()
{
    auto path = QFileDialog::getSaveFileName(
        this, "Export clean SVG",
        suggested_output_path(export_directory_setting, current_file_path_, "svg"),
        "SVG image (*.svg)");
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(".svg", Qt::CaseInsensitive)) {
        path += ".svg";
    }
    const auto absolute_path = QFileInfo(path).absoluteFilePath();
    if (!current_file_path_.isEmpty() && absolute_path == current_file_path_) {
        QMessageBox::warning(
            this,
            "Choose another export file",
            "A clean SVG cannot replace the current editable RosetteLab project. Choose another filename.");
        return;
    }
    remember_selected_directory(export_directory_setting, absolute_path);

    std::string svg_text;
    try {
        svg_text = svg::serialize_clean_svg(document_);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Unable to export", error.what());
        return;
    }
    QFile file(absolute_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Unable to export", file.errorString());
        return;
    }
    const auto data = QByteArray::fromStdString(svg_text);
    if (file.write(data) != data.size()) {
        QMessageBox::critical(this, "Unable to export", file.errorString());
    }
}

void MainWindow::export_raster(const bool jpeg)
{
    bool accepted = false;
    const int dpi = QInputDialog::getInt(
        this, jpeg ? "Export JPEG" : "Export PNG", "Resolution", 300, 72, 1200, 1, &accepted);
    if (!accepted) {
        return;
    }

    constexpr double millimetres_per_inch = 25.4;
    const auto width = static_cast<int>(std::lround(
        document_.settings().page_width / millimetres_per_inch * dpi));
    const auto height = static_cast<int>(std::lround(
        document_.settings().page_height / millimetres_per_inch * dpi));
    constexpr qint64 maximum_pixels = 100'000'000;
    if (width <= 0 || height <= 0 || width > 32767 || height > 32767 ||
        static_cast<qint64>(width) * height > maximum_pixels) {
        QMessageBox::warning(
            this, "Export dimensions too large",
            QStringLiteral("The requested export would be %1 × %2 pixels. Reduce the DPI or page size.")
                .arg(width).arg(height));
        return;
    }

    auto path = QFileDialog::getSaveFileName(
        this,
        jpeg ? "Export JPEG" : "Export PNG",
        suggested_output_path(
            export_directory_setting, current_file_path_, jpeg ? "jpg" : "png"),
        jpeg ? "JPEG image (*.jpg)" : "PNG image (*.png)");
    if (path.isEmpty()) {
        return;
    }
    if (jpeg && path.endsWith(".jpeg", Qt::CaseInsensitive)) {
        path.chop(5);
        path += ".jpg";
    } else if (jpeg && !path.endsWith(".jpg", Qt::CaseInsensitive)) {
        path += ".jpg";
    } else if (!jpeg && !path.endsWith(".png", Qt::CaseInsensitive)) {
        path += ".png";
    }
    remember_selected_directory(export_directory_setting, path);

    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(jpeg ? Qt::white : Qt::transparent);
    image.setDotsPerMeterX(static_cast<int>(std::lround(dpi / 0.0254)));
    image.setDotsPerMeterY(static_cast<int>(std::lround(dpi / 0.0254)));
    {
        QPainter painter(&image);
        render::render_document(painter, document_, QRectF(0, 0, width, height));
    }
    if (!image.save(path, jpeg ? "JPEG" : "PNG", jpeg ? 95 : -1)) {
        QMessageBox::critical(this, "Unable to export", "Qt could not write the selected image file.");
    }
}

void MainWindow::export_pdf()
{
    QDialog options_dialog(this);
    options_dialog.setWindowTitle("Export PDF");
    auto* options_layout = new QVBoxLayout(&options_dialog);
    auto* options_form = new QFormLayout;
    auto* rendering_control = new QComboBox(&options_dialog);
    rendering_control->addItems({"Preserve vector blend modes", "Rasterize for compatibility"});
    options_form->addRow("Rendering", rendering_control);
    auto* color_model_control = new QComboBox(&options_dialog);
    color_model_control->addItem("RGB");
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    color_model_control->addItem("CMYK (generic, no ICC profile)");
#endif
    options_form->addRow("Color model", color_model_control);
    options_layout->addLayout(options_form);
    auto* options_buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &options_dialog);
    options_layout->addWidget(options_buttons);
    connect(options_buttons, &QDialogButtonBox::accepted, &options_dialog, &QDialog::accept);
    connect(options_buttons, &QDialogButtonBox::rejected, &options_dialog, &QDialog::reject);
    if (options_dialog.exec() != QDialog::Accepted) {
        return;
    }
    const bool preserve_vector_blends = rendering_control->currentIndex() == 0;
    const bool use_cmyk = color_model_control->currentIndex() == 1;

    auto path = QFileDialog::getSaveFileName(
        this, "Export PDF",
        suggested_output_path(export_directory_setting, current_file_path_, "pdf"),
        "PDF document (*.pdf)");
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(".pdf", Qt::CaseInsensitive)) {
        path += ".pdf";
    }
    remember_selected_directory(export_directory_setting, path);

    if (preserve_vector_blends) {
        pdf::ExportOptions options;
        options.color_model = use_cmyk ? pdf::ColorModel::Cmyk : pdf::ColorModel::Rgb;
        std::string bytes;
        try {
            bytes = pdf::serialize_vector_pdf(document_, options);
        } catch (const std::exception& error) {
            QMessageBox::critical(this, "Unable to export", QString::fromUtf8(error.what()));
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)
            || file.write(bytes.data(), static_cast<qint64>(bytes.size())) != static_cast<qint64>(bytes.size())) {
            QMessageBox::critical(this, "Unable to export", "RosetteLab could not write the selected PDF file.");
        }
        return;
    }

    QPdfWriter writer(path);
    writer.setTitle("RosetteLab export");
    writer.setCreator("RosetteLab");
    writer.setResolution(300);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    if (use_cmyk) {
        writer.setColorModel(QPdfWriter::ColorModel::CMYK);
    } else {
        writer.setColorModel(QPdfWriter::ColorModel::RGB);
        writer.setOutputIntent(QPdfOutputIntent{});
    }
#endif
    const QPageSize page_size(
        QSizeF(document_.settings().page_width, document_.settings().page_height),
        QPageSize::Millimeter,
        "RosetteLab page",
        QPageSize::ExactMatch);
    writer.setPageLayout(QPageLayout(
        page_size, QPageLayout::Portrait, QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter));

    QPainter painter;
    if (!painter.begin(&writer)) {
        QMessageBox::critical(this, "Unable to export", "Qt could not create the selected PDF file.");
        return;
    }
    const QRectF pdf_page(0, 0, writer.width(), writer.height());
    // Explicit compatibility mode always precomposes the complete page.
    QImage composed(writer.width(), writer.height(), QImage::Format_ARGB32_Premultiplied);
    composed.fill(Qt::transparent);
    {
        QPainter image_painter(&composed);
        render::render_document(image_painter, document_, composed.rect());
    }
    painter.drawImage(pdf_page, composed);
    painter.end();
}

void MainWindow::add_polar_rose()
{
    const auto suggested = QString::fromStdString(
        document_.suggested_default_name(document::CurveType::PolarRose));
    bool accepted = false;
    const auto name = QInputDialog::getText(
        this, "New polar rose", "Layer name", QLineEdit::Normal, suggested, &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return;
    }

    auto& layer = document_.add_polar_rose({}, name.toStdString());
    layer.preset_id = "rose-seven";
    auto* item = add_layer_row(layer);
    layers_->setCurrentItem(item);
    preview_->update();
    mark_document_modified();
}

void MainWindow::add_ellipse()
{
    const auto suggested = QString::fromStdString(
        document_.suggested_default_name(document::CurveType::Ellipse));
    bool accepted = false;
    const auto name = QInputDialog::getText(
        this, "New ellipse", "Layer name", QLineEdit::Normal, suggested, &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return;
    }

    auto& layer = document_.add_ellipse({}, name.toStdString());
    auto* item = add_layer_row(layer);
    layers_->setCurrentItem(item);
    preview_->update();
    mark_document_modified();
}

void MainWindow::add_trochoid(const document::CurveType type)
{
    const auto type_name = QString::fromStdString(document::curve_type_name(type));
    const auto suggested = QString::fromStdString(document_.suggested_default_name(type));
    bool accepted = false;
    const auto name = QInputDialog::getText(
        this, QStringLiteral("New %1").arg(type_name), "Layer name",
        QLineEdit::Normal, suggested, &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return;
    }
    auto& layer = document_.add_trochoid(type, {}, name.toStdString());
    auto* item = add_layer_row(layer);
    layers_->setCurrentItem(item);
    preview_->update();
    mark_document_modified();
}

void MainWindow::add_lissajous()
{
    const auto suggested = QString::fromStdString(
        document_.suggested_default_name(document::CurveType::Lissajous));
    bool accepted = false;
    const auto name = QInputDialog::getText(
        this, "New Lissajous curve", "Layer name", QLineEdit::Normal,
        suggested, &accepted).trimmed();
    if (!accepted || name.isEmpty()) return;
    auto& layer = document_.add_lissajous({}, name.toStdString());
    layer.preset_id = "liss-3-2";
    auto* item = add_layer_row(layer);
    layers_->setCurrentItem(item);
    preview_->update();
    mark_document_modified();
}

void MainWindow::add_harmonograph()
{
    const auto suggested=QString::fromStdString(document_.suggested_default_name(document::CurveType::Harmonograph));
    bool accepted=false;
    const auto name=QInputDialog::getText(this,"New Harmonograph curve","Layer name",QLineEdit::Normal,suggested,&accepted).trimmed();
    if (!accepted||name.isEmpty()) return;
    auto& layer=document_.add_harmonograph({},name.toStdString());
    auto* item=add_layer_row(layer); layers_->setCurrentItem(item); preview_->update(); mark_document_modified();
}

void MainWindow::add_droplet_rosette()
{
    const auto suggested = QString::fromStdString(
        document_.suggested_default_name(document::CurveType::DropletRosette));
    bool accepted = false;
    const auto name = QInputDialog::getText(
        this, "New Droplet Rosette", "Layer name", QLineEdit::Normal,
        suggested, &accepted).trimmed();
    if (!accepted || name.isEmpty()) return;
    auto& layer = document_.add_droplet_rosette({}, name.toStdString());
    auto* item = add_layer_row(layer);
    layers_->setCurrentItem(item);
    preview_->update();
    mark_document_modified();
}

QListWidgetItem* MainWindow::add_layer_row(const document::CurveLayer& layer, const int row)
{
    auto* item = new QListWidgetItem;
    item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(layer.id));
    item->setSizeHint(QSize(220, 32));
    if (row < 0) {
        layers_->insertItem(0, item);
    } else {
        layers_->insertItem(row, item);
    }

    auto* widget = new LayerListItemWidget(
        layer.id,
        QString::fromStdString(layer.name),
        layer.visible,
        layer.locked,
        [this, id = layer.id] { select_layer_row(id); },
        [this, id = layer.id](const bool visible) {
            select_layer_row(id);
            static_cast<void>(document_.set_layer_visible(id, visible));
            preview_->update();
            mark_document_modified();
        },
        [this, id = layer.id](const bool locked) {
            select_layer_row(id);
            set_active_layer_locked(locked);
        });
    layers_->setItemWidget(item, widget);
    widget->set_layer_preview(layer, document_.settings().background);
    return item;
}

void MainWindow::select_layer_row(const document::LayerId id)
{
    for (int row = 0; row < layers_->count(); ++row) {
        auto* item = layers_->item(row);
        if (item->data(Qt::UserRole).toULongLong() == id) {
            layers_->setCurrentItem(item);
            return;
        }
    }
}

void MainWindow::select_layer(const document::LayerId id)
{
    history_coalescing_key_.clear();
    history_coalescing_timer_.invalidate();
    active_layer_id_ = id;
    load_active_layer();
    refresh_layer_actions();
}

void MainWindow::load_active_layer()
{
    const auto* layer = document_.find_layer(active_layer_id_);
    if (layer == nullptr) {
        return;
    }
    curve_type_label_->setText(QString::fromStdString(document::curve_type_name(layer->type)));
    refresh_preset_choices();
    const auto* parameters = std::get_if<curves::PolarRoseParameters>(&layer->parameters);
    const auto* ellipse_parameters = std::get_if<curves::EllipseParameters>(&layer->parameters);
    const auto* trochoid_parameters = std::get_if<curves::TrochoidParameters>(&layer->parameters);
    const auto* lissajous_parameters = std::get_if<curves::LissajousParameters>(&layer->parameters);
    const auto* harmonograph_parameters = std::get_if<curves::HarmonographParameters>(&layer->parameters);
    const auto* droplet_parameters = std::get_if<curves::DropletRosetteParameters>(&layer->parameters);
    curve_group_->setVisible(parameters != nullptr);
    ellipse_group_->setVisible(ellipse_parameters != nullptr);
    trochoid_group_->setVisible(trochoid_parameters != nullptr);
    lissajous_group_->setVisible(lissajous_parameters != nullptr);
    harmonograph_group_->setVisible(harmonograph_parameters != nullptr);
    droplet_group_->setVisible(droplet_parameters != nullptr);

    const QSignalBlocker radius_blocker(radius_);
    const QSignalBlocker k_mode_blocker(k_mode_);
    const QSignalBlocker k_blocker(k_);
    const QSignalBlocker numerator_blocker(numerator_);
    const QSignalBlocker denominator_blocker(denominator_);
    const QSignalBlocker phase_blocker(phase_);
    const QSignalBlocker rotation_blocker(rotation_);
    const QSignalBlocker tolerance_blocker(tolerance_);
    const QSignalBlocker ellipse_radius_x_blocker(ellipse_radius_x_);
    const QSignalBlocker ellipse_radius_y_blocker(ellipse_radius_y_);
    const QSignalBlocker ellipse_link_blocker(ellipse_link_radii_);
    const QSignalBlocker ellipse_rotation_blocker(ellipse_rotation_);
    const QSignalBlocker ellipse_tolerance_blocker(ellipse_tolerance_);
    const QSignalBlocker trochoid_fixed_blocker(trochoid_fixed_radius_);
    const QSignalBlocker trochoid_rolling_blocker(trochoid_rolling_radius_);
    const QSignalBlocker trochoid_offset_blocker(trochoid_pen_offset_);
    const QSignalBlocker trochoid_rotation_blocker(trochoid_rotation_);
    const QSignalBlocker trochoid_mode_blocker(trochoid_trace_mode_);
    const QSignalBlocker trochoid_turns_blocker(trochoid_turns_);
    const QSignalBlocker trochoid_close_blocker(trochoid_close_limited_);
    const QSignalBlocker trochoid_tolerance_blocker(trochoid_tolerance_);
    const QSignalBlocker liss_ax_blocker(lissajous_amplitude_x_);
    const QSignalBlocker liss_ay_blocker(lissajous_amplitude_y_);
    const QSignalBlocker liss_fx_blocker(lissajous_frequency_x_);
    const QSignalBlocker liss_fy_blocker(lissajous_frequency_y_);
    const QSignalBlocker liss_px_blocker(lissajous_phase_x_);
    const QSignalBlocker liss_py_blocker(lissajous_phase_y_);
    const QSignalBlocker liss_rotation_blocker(lissajous_rotation_);
    const QSignalBlocker liss_tolerance_blocker(lissajous_tolerance_);
    const QSignalBlocker harm_ax(harmonograph_amplitude_x_),harm_ay(harmonograph_amplitude_y_),harm_fx(harmonograph_frequency_x_),harm_fy(harmonograph_frequency_y_),harm_px(harmonograph_phase_x_),harm_py(harmonograph_phase_y_),harm_dx(harmonograph_damping_x_),harm_dy(harmonograph_damping_y_),harm_duration(harmonograph_duration_),harm_rotation(harmonograph_rotation_),harm_tolerance(harmonograph_tolerance_);
    const QSignalBlocker drop_count(droplet_count_), drop_outer(droplet_outer_radius_),
        drop_core(droplet_core_radius_), drop_swirl(droplet_swirl_), drop_width(droplet_width_),
        drop_roundness(droplet_roundness_), drop_rotation(droplet_rotation_);
    const QSignalBlocker transform_x_blocker(transform_x_);
    const QSignalBlocker transform_y_blocker(transform_y_);
    const QSignalBlocker transform_scale_x_blocker(transform_scale_x_);
    const QSignalBlocker transform_scale_y_blocker(transform_scale_y_);
    const QSignalBlocker transform_link_blocker(transform_link_scales_);
    const QSignalBlocker transform_rotation_blocker(transform_rotation_);
    const QSignalBlocker copy_arrangement_blocker(copy_arrangement_);
    const QSignalBlocker copy_count_blocker(copy_count_);
    const QSignalBlocker copy_rotation_blocker(copy_rotation_);
    const QSignalBlocker copy_scale_blocker(copy_scale_);
    const QSignalBlocker copy_offset_x_blocker(copy_offset_x_);
    const QSignalBlocker copy_offset_y_blocker(copy_offset_y_);
    const QSignalBlocker copy_circular_radius_blocker(copy_circular_radius_);
    const QSignalBlocker copy_circular_start_blocker(copy_circular_start_);
    const QSignalBlocker copy_circular_angle_blocker(copy_circular_angle_);
    const QSignalBlocker copy_orbit_blocker(copy_rotate_with_orbit_);
    const QSignalBlocker stroke_enabled_blocker(stroke_enabled_);
    const QSignalBlocker stroke_width_blocker(stroke_width_);
    const QSignalBlocker fill_enabled_blocker(fill_enabled_);
    const QSignalBlocker fill_rule_blocker(fill_rule_);
    const QSignalBlocker opacity_blocker(layer_opacity_);
    const QSignalBlocker blend_blocker(blend_mode_);
    if (parameters != nullptr) {
        radius_->setValue(parameters->radius);
        k_mode_->setCurrentIndex(k_mode_->findData(static_cast<int>(parameters->k_mode)));
        k_->setValue(parameters->k);
        numerator_->setValue(parameters->numerator);
        denominator_->setValue(parameters->denominator);
        phase_->setValue(parameters->phase_degrees);
        rotation_->setValue(parameters->rotation_degrees);
        tolerance_->setValue(parameters->bezier_tolerance);
    } else if (ellipse_parameters != nullptr) {
        ellipse_radius_x_->setValue(ellipse_parameters->radius_x);
        ellipse_radius_y_->setValue(ellipse_parameters->radius_y);
        ellipse_link_radii_->setChecked(ellipse_parameters->link_radii);
        ellipse_rotation_->setValue(ellipse_parameters->rotation_degrees);
        ellipse_tolerance_->setValue(ellipse_parameters->bezier_tolerance);
    } else if (trochoid_parameters != nullptr) {
        trochoid_fixed_radius_->setValue(trochoid_parameters->fixed_radius);
        trochoid_rolling_radius_->setValue(trochoid_parameters->rolling_radius);
        trochoid_pen_offset_->setValue(trochoid_parameters->pen_offset);
        trochoid_rotation_->setValue(trochoid_parameters->rotation_degrees);
        trochoid_trace_mode_->setCurrentIndex(trochoid_trace_mode_->findData(
            static_cast<int>(trochoid_parameters->trace_mode)));
        trochoid_turns_->setValue(trochoid_parameters->turns);
        trochoid_close_limited_->setChecked(trochoid_parameters->close_limited_path);
        trochoid_tolerance_->setValue(trochoid_parameters->bezier_tolerance);
    } else if (lissajous_parameters != nullptr) {
        lissajous_amplitude_x_->setValue(lissajous_parameters->amplitude_x);
        lissajous_amplitude_y_->setValue(lissajous_parameters->amplitude_y);
        lissajous_frequency_x_->setValue(lissajous_parameters->frequency_x);
        lissajous_frequency_y_->setValue(lissajous_parameters->frequency_y);
        lissajous_phase_x_->setValue(lissajous_parameters->phase_x_degrees);
        lissajous_phase_y_->setValue(lissajous_parameters->phase_y_degrees);
        lissajous_rotation_->setValue(lissajous_parameters->rotation_degrees);
        lissajous_tolerance_->setValue(lissajous_parameters->bezier_tolerance);
    } else if (harmonograph_parameters != nullptr) {
        harmonograph_amplitude_x_->setValue(harmonograph_parameters->amplitude_x); harmonograph_amplitude_y_->setValue(harmonograph_parameters->amplitude_y);
        harmonograph_frequency_x_->setValue(harmonograph_parameters->frequency_x); harmonograph_frequency_y_->setValue(harmonograph_parameters->frequency_y);
        harmonograph_phase_x_->setValue(harmonograph_parameters->phase_x_degrees); harmonograph_phase_y_->setValue(harmonograph_parameters->phase_y_degrees);
        harmonograph_damping_x_->setValue(harmonograph_parameters->damping_x); harmonograph_damping_y_->setValue(harmonograph_parameters->damping_y);
        harmonograph_duration_->setValue(harmonograph_parameters->duration); harmonograph_rotation_->setValue(harmonograph_parameters->rotation_degrees); harmonograph_tolerance_->setValue(harmonograph_parameters->bezier_tolerance);
    } else if (droplet_parameters != nullptr) {
        droplet_count_->setValue(droplet_parameters->droplets);
        droplet_outer_radius_->setValue(droplet_parameters->outer_radius);
        droplet_core_radius_->setValue(droplet_parameters->core_radius);
        droplet_swirl_->setValue(droplet_parameters->swirl_degrees);
        droplet_width_->setValue(droplet_parameters->width_percent);
        droplet_roundness_->setValue(droplet_parameters->roundness);
        droplet_rotation_->setValue(droplet_parameters->rotation_degrees);
    }
    transform_x_->setValue(layer->transform.position_x);
    transform_y_->setValue(layer->transform.position_y);
    transform_scale_x_->setValue(layer->transform.scale_x * 100.0);
    transform_scale_y_->setValue(layer->transform.scale_y * 100.0);
    transform_link_scales_->setChecked(layer->transform.link_scales);
    transform_rotation_->setValue(layer->transform.rotation_degrees);
    copy_arrangement_->setCurrentIndex(copy_arrangement_->findData(
        static_cast<int>(layer->copies.arrangement)));
    copy_count_->setValue(layer->copies.count);
    copy_rotation_->setValue(layer->copies.rotation_step_degrees);
    copy_scale_->setValue(layer->copies.scale_step * 100.0);
    copy_offset_x_->setValue(layer->copies.offset_x_step);
    copy_offset_y_->setValue(layer->copies.offset_y_step);
    copy_circular_radius_->setValue(layer->copies.circular_radius);
    copy_circular_start_->setValue(layer->copies.circular_start_degrees);
    copy_circular_angle_->setValue(layer->copies.circular_angle_step_degrees);
    copy_rotate_with_orbit_->setChecked(layer->copies.rotate_with_orbit);
    stroke_color_ = qcolor_from_rgba(layer->appearance.stroke);
    fill_color_ = qcolor_from_rgba(layer->appearance.fill);
    stroke_enabled_->setChecked(layer->appearance.stroke_enabled);
    stroke_width_->setValue(layer->appearance.stroke_width);
    fill_enabled_->setChecked(layer->appearance.fill_enabled);
    fill_rule_->setCurrentIndex(fill_rule_->findData(static_cast<int>(layer->appearance.fill_rule)));
    layer_opacity_->setValue(static_cast<int>(std::lround(layer->appearance.opacity * 100.0)));
    blend_mode_->setCurrentIndex(blend_mode_->findData(static_cast<int>(layer->appearance.blend_mode)));
    refresh_color_buttons();
    refresh_k_mode_controls();
    refresh_ellipse_radius_controls();
    refresh_trochoid_trace_controls();
    refresh_transform_controls();
    refresh_copy_controls();
}

void MainWindow::refresh_k_mode_controls()
{
    const auto mode = static_cast<curves::PolarKMode>(k_mode_->currentData().toInt());
    const bool decimal = mode == curves::PolarKMode::Decimal;
    k_->setEnabled(decimal);
    numerator_->setEnabled(!decimal);
    denominator_->setEnabled(!decimal);
}

void MainWindow::refresh_ellipse_radius_controls()
{
    ellipse_radius_y_->setEnabled(!ellipse_link_radii_->isChecked());
}

void MainWindow::refresh_trochoid_trace_controls()
{
    const bool limited = static_cast<curves::TraceMode>(
        trochoid_trace_mode_->currentData().toInt()) == curves::TraceMode::Limited;
    trochoid_turns_->setEnabled(limited);
    trochoid_close_limited_->setEnabled(limited);
}

void MainWindow::refresh_transform_controls()
{
    transform_scale_y_->setEnabled(!transform_link_scales_->isChecked());
}

void MainWindow::refresh_copy_controls()
{
    const auto arrangement = static_cast<document::CopyArrangement>(
        copy_arrangement_->currentData().toInt());
    const bool linear = arrangement == document::CopyArrangement::Linear;
    const bool circular = arrangement == document::CopyArrangement::Circular;
    copy_offset_x_->setEnabled(linear);
    copy_offset_y_->setEnabled(linear);
    copy_circular_radius_->setEnabled(circular);
    copy_circular_start_->setEnabled(circular);
    copy_circular_angle_->setEnabled(circular);
    copy_rotate_with_orbit_->setEnabled(circular);
    distribute_copies_button_->setEnabled(circular && copy_count_->value() > 0);
}

void MainWindow::update_layer_transform(const QString& coalescing_key)
{
    auto* layer = document_.find_layer(active_layer_id_);
    if (layer == nullptr || layer->locked) {
        return;
    }
    layer->transform.position_x = transform_x_->value();
    layer->transform.position_y = transform_y_->value();
    layer->transform.scale_x = transform_scale_x_->value() / 100.0;
    layer->transform.link_scales = transform_link_scales_->isChecked();
    layer->transform.scale_y = layer->transform.link_scales
        ? layer->transform.scale_x
        : transform_scale_y_->value() / 100.0;
    layer->transform.rotation_degrees = transform_rotation_->value();
    layer->copies.arrangement = static_cast<document::CopyArrangement>(
        copy_arrangement_->currentData().toInt());
    layer->copies.count = copy_count_->value();
    layer->copies.rotation_step_degrees = copy_rotation_->value();
    layer->copies.scale_step = copy_scale_->value() / 100.0;
    layer->copies.offset_x_step = copy_offset_x_->value();
    layer->copies.offset_y_step = copy_offset_y_->value();
    layer->copies.circular_radius = copy_circular_radius_->value();
    layer->copies.circular_start_degrees = copy_circular_start_->value();
    layer->copies.circular_angle_step_degrees = copy_circular_angle_->value();
    layer->copies.rotate_with_orbit = copy_rotate_with_orbit_->isChecked();
    refresh_layer_preview(active_layer_id_);
    preview_->update();
    mark_document_modified(coalescing_key);
}

void MainWindow::reset_layer_transform()
{
    const QSignalBlocker x_blocker(transform_x_);
    const QSignalBlocker y_blocker(transform_y_);
    const QSignalBlocker scale_x_blocker(transform_scale_x_);
    const QSignalBlocker scale_y_blocker(transform_scale_y_);
    const QSignalBlocker linked_blocker(transform_link_scales_);
    const QSignalBlocker rotation_blocker(transform_rotation_);
    transform_x_->setValue(0.0);
    transform_y_->setValue(0.0);
    transform_scale_x_->setValue(100.0);
    transform_scale_y_->setValue(100.0);
    transform_link_scales_->setChecked(true);
    transform_rotation_->setValue(0.0);
    refresh_transform_controls();
    update_layer_transform();
}

void MainWindow::reset_layer_copies()
{
    const QSignalBlocker arrangement_blocker(copy_arrangement_);
    const QSignalBlocker count_blocker(copy_count_);
    const QSignalBlocker rotation_blocker(copy_rotation_);
    const QSignalBlocker scale_blocker(copy_scale_);
    const QSignalBlocker offset_x_blocker(copy_offset_x_);
    const QSignalBlocker offset_y_blocker(copy_offset_y_);
    const QSignalBlocker radius_blocker(copy_circular_radius_);
    const QSignalBlocker start_blocker(copy_circular_start_);
    const QSignalBlocker angle_blocker(copy_circular_angle_);
    const QSignalBlocker orbit_blocker(copy_rotate_with_orbit_);
    copy_arrangement_->setCurrentIndex(copy_arrangement_->findData(
        static_cast<int>(document::CopyArrangement::Superimposed)));
    copy_count_->setValue(1);
    copy_rotation_->setValue(0.0);
    copy_scale_->setValue(100.0);
    copy_offset_x_->setValue(0.0);
    copy_offset_y_->setValue(0.0);
    copy_circular_radius_->setValue(0.0);
    copy_circular_start_->setValue(0.0);
    copy_circular_angle_->setValue(0.0);
    copy_rotate_with_orbit_->setChecked(true);
    refresh_copy_controls();
    update_layer_transform();
}

void MainWindow::distribute_copies_over_circle()
{
    if (copy_count_->value() < 1) {
        return;
    }
    const QSignalBlocker blocker(copy_circular_angle_);
    copy_circular_angle_->setValue(360.0 / copy_count_->value());
    update_layer_transform();
}

void MainWindow::choose_stroke_color()
{
    ColorEditorDialog dialog(stroke_color_, "Stroke color", this);
    if (dialog.exec() == QDialog::Accepted) {
        stroke_color_ = dialog.color();
        refresh_color_buttons();
        update_appearance();
    }
}

void MainWindow::choose_fill_color()
{
    ColorEditorDialog dialog(fill_color_, "Fill color", this);
    if (dialog.exec() == QDialog::Accepted) {
        fill_color_ = dialog.color();
        refresh_color_buttons();
        update_appearance();
    }
}

void MainWindow::update_appearance(const QString& coalescing_key)
{
    auto* layer = document_.find_layer(active_layer_id_);
    if (layer == nullptr || layer->locked) {
        return;
    }
    layer->appearance.stroke = rgba_from_qcolor(stroke_color_);
    layer->appearance.stroke_enabled = stroke_enabled_->isChecked();
    layer->appearance.stroke_width = stroke_width_->value();
    layer->appearance.fill_enabled = fill_enabled_->isChecked();
    layer->appearance.fill = rgba_from_qcolor(fill_color_);
    layer->appearance.fill_rule = static_cast<document::FillRule>(fill_rule_->currentData().toInt());
    layer->appearance.opacity = static_cast<double>(layer_opacity_->value()) / 100.0;
    layer->appearance.blend_mode = static_cast<document::BlendMode>(blend_mode_->currentData().toInt());
    refresh_color_buttons();
    refresh_layer_preview(active_layer_id_);
    preview_->update();
    if (!applying_preset_) mark_document_modified(coalescing_key);
}

void MainWindow::refresh_color_buttons()
{
    style_color_button(stroke_color_button_, stroke_color_);
    style_color_button(fill_color_button_, fill_color_);
    stroke_color_button_->setEnabled(stroke_enabled_->isChecked());
    stroke_width_->setEnabled(stroke_enabled_->isChecked());
    const bool fill_controls_enabled = fill_enabled_->isChecked();
    fill_color_button_->setEnabled(fill_controls_enabled);
    fill_rule_->setEnabled(fill_controls_enabled);
}

void MainWindow::rename_active_layer()
{
    auto* layer = document_.find_layer(active_layer_id_);
    auto* item = layers_->currentItem();
    if (layer == nullptr || item == nullptr) {
        return;
    }

    bool accepted = false;
    const auto name = QInputDialog::getText(
        this,
        "Rename layer",
        "Layer name",
        QLineEdit::Normal,
        QString::fromStdString(layer->name),
        &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return;
    }
    if (document_.rename_layer(active_layer_id_, name.toStdString())) {
        auto* widget = static_cast<LayerListItemWidget*>(layers_->itemWidget(item));
        if (widget != nullptr) {
            widget->set_name(name);
        }
        mark_document_modified();
    }
}

void MainWindow::duplicate_active_layer()
{
    const auto source_row = layers_->currentRow();
    auto* duplicate = document_.duplicate_layer(active_layer_id_);
    if (duplicate == nullptr) {
        return;
    }

    auto* item = add_layer_row(*duplicate, source_row);
    layers_->setCurrentItem(item);
    preview_->update();
    mark_document_modified();
}

void MainWindow::delete_active_layer()
{
    auto* layer = document_.find_layer(active_layer_id_);
    const int row = layers_->currentRow();
    if (layer == nullptr || row < 0) {
        return;
    }
    const auto answer = QMessageBox::question(
        this,
        "Delete layer",
        QStringLiteral("Delete layer \"%1\"?").arg(QString::fromStdString(layer->name)));
    if (answer != QMessageBox::Yes) {
        return;
    }

    static_cast<void>(document_.remove_layer(active_layer_id_));
    delete layers_->takeItem(row);
    if (layers_->count() > 0) {
        layers_->setCurrentRow(std::min(row, layers_->count() - 1));
    } else {
        active_layer_id_ = 0;
    }
    preview_->update();
    refresh_layer_actions();
    mark_document_modified();
}

void MainWindow::set_active_layer_locked(const bool locked)
{
    if (document_.set_layer_locked(active_layer_id_, locked)) {
        refresh_layer_actions();
        mark_document_modified();
    }
}

void MainWindow::refresh_layer_actions()
{
    const auto* layer = document_.find_layer(active_layer_id_);
    const bool has_layer = layer != nullptr;
    rename_button_->setEnabled(has_layer);
    duplicate_button_->setEnabled(has_layer);
    delete_button_->setEnabled(has_layer);
    curve_group_->setEnabled(has_layer && !layer->locked);
    ellipse_group_->setEnabled(has_layer && !layer->locked);
    trochoid_group_->setEnabled(has_layer && !layer->locked);
    lissajous_group_->setEnabled(has_layer && !layer->locked);
    harmonograph_group_->setEnabled(has_layer && !layer->locked);
    transform_group_->setEnabled(has_layer && !layer->locked);
    copies_group_->setEnabled(has_layer && !layer->locked);
    appearance_group_->setEnabled(has_layer && !layer->locked);
}

void MainWindow::refresh_layer_preview(const document::LayerId id)
{
    const auto* layer = document_.find_layer(id);
    if (layer == nullptr) {
        return;
    }
    for (int row = 0; row < layers_->count(); ++row) {
        auto* item = layers_->item(row);
        if (item->data(Qt::UserRole).toULongLong() != id) {
            continue;
        }
        auto* widget = static_cast<LayerListItemWidget*>(layers_->itemWidget(item));
        if (widget != nullptr) {
            widget->set_layer_preview(*layer, document_.settings().background);
        }
        return;
    }
}

void MainWindow::refresh_all_layer_previews()
{
    for (const auto& layer : document_.layers()) {
        refresh_layer_preview(layer.id);
    }
}

void MainWindow::sync_layer_order()
{
    for (int target = 0; target < layers_->count(); ++target) {
        const auto id = static_cast<document::LayerId>(
            layers_->item(layers_->count() - 1 - target)->data(Qt::UserRole).toULongLong());
        const auto& layers = document_.layers();
        const auto iterator = std::find_if(layers.begin(), layers.end(),
            [id](const document::CurveLayer& layer) { return layer.id == id; });
        if (iterator == layers.end()) {
            continue;
        }
        const auto current = static_cast<std::size_t>(std::distance(layers.begin(), iterator));
        static_cast<void>(document_.move_layer(current, static_cast<std::size_t>(target)));
    }
    preview_->update();
    mark_document_modified();
}

void MainWindow::update_preview(const QString& coalescing_key)
{
    auto* layer = document_.find_layer(active_layer_id_);
    if (layer == nullptr || layer->locked) {
        return;
    }
    if (!applying_preset_ && preset_->currentIndex() > 0) {
        const QSignalBlocker blocker(preset_);
        preset_->setCurrentIndex(0);
        layer->preset_customized = true;
    }
    if (auto* parameters = std::get_if<curves::PolarRoseParameters>(&layer->parameters)) {
        parameters->radius = radius_->value();
        parameters->k_mode = static_cast<curves::PolarKMode>(k_mode_->currentData().toInt());
        parameters->k = k_->value();
        parameters->numerator = numerator_->value();
        parameters->denominator = denominator_->value();
        parameters->phase_degrees = phase_->value();
        parameters->rotation_degrees = rotation_->value();
        parameters->bezier_tolerance = tolerance_->value();
    } else if (auto* parameters = std::get_if<curves::EllipseParameters>(&layer->parameters)) {
        parameters->radius_x = ellipse_radius_x_->value();
        parameters->link_radii = ellipse_link_radii_->isChecked();
        parameters->radius_y = parameters->link_radii
            ? parameters->radius_x
            : ellipse_radius_y_->value();
        parameters->rotation_degrees = ellipse_rotation_->value();
        parameters->bezier_tolerance = ellipse_tolerance_->value();
    } else if (auto* parameters = std::get_if<curves::TrochoidParameters>(&layer->parameters)) {
        parameters->fixed_radius = trochoid_fixed_radius_->value();
        parameters->rolling_radius = trochoid_rolling_radius_->value();
        parameters->pen_offset = trochoid_pen_offset_->value();
        parameters->rotation_degrees = trochoid_rotation_->value();
        parameters->trace_mode = static_cast<curves::TraceMode>(
            trochoid_trace_mode_->currentData().toInt());
        parameters->turns = trochoid_turns_->value();
        parameters->close_limited_path = trochoid_close_limited_->isChecked();
        parameters->bezier_tolerance = trochoid_tolerance_->value();
    } else if (auto* parameters = std::get_if<curves::LissajousParameters>(&layer->parameters)) {
        parameters->amplitude_x = lissajous_amplitude_x_->value();
        parameters->amplitude_y = lissajous_amplitude_y_->value();
        parameters->frequency_x = lissajous_frequency_x_->value();
        parameters->frequency_y = lissajous_frequency_y_->value();
        parameters->phase_x_degrees = lissajous_phase_x_->value();
        parameters->phase_y_degrees = lissajous_phase_y_->value();
        parameters->rotation_degrees = lissajous_rotation_->value();
        parameters->bezier_tolerance = lissajous_tolerance_->value();
    } else if (auto* parameters = std::get_if<curves::HarmonographParameters>(&layer->parameters)) {
        parameters->amplitude_x=harmonograph_amplitude_x_->value(); parameters->amplitude_y=harmonograph_amplitude_y_->value();
        parameters->frequency_x=harmonograph_frequency_x_->value(); parameters->frequency_y=harmonograph_frequency_y_->value();
        parameters->phase_x_degrees=harmonograph_phase_x_->value(); parameters->phase_y_degrees=harmonograph_phase_y_->value();
        parameters->damping_x=harmonograph_damping_x_->value(); parameters->damping_y=harmonograph_damping_y_->value();
        parameters->duration=harmonograph_duration_->value(); parameters->rotation_degrees=harmonograph_rotation_->value(); parameters->bezier_tolerance=harmonograph_tolerance_->value();
    } else if (auto* parameters = std::get_if<curves::DropletRosetteParameters>(&layer->parameters)) {
        parameters->droplets = droplet_count_->value();
        parameters->outer_radius = droplet_outer_radius_->value();
        parameters->core_radius = std::min(droplet_core_radius_->value(), parameters->outer_radius * 0.999);
        parameters->swirl_degrees = droplet_swirl_->value();
        parameters->width_percent = droplet_width_->value();
        parameters->roundness = droplet_roundness_->value();
        parameters->rotation_degrees = droplet_rotation_->value();
    }
    refresh_layer_preview(active_layer_id_);
    preview_->update();
    mark_document_modified(coalescing_key);
}

void MainWindow::refresh_preset_choices()
{
    const auto* layer=document_.find_layer(active_layer_id_);
    const QSignalBlocker blocker(preset_);
    preset_->clear(); preset_->addItem("Custom", "");
    if (layer==nullptr) { restore_preset_button_->setEnabled(false); return; }
    const auto add=[this](const char* name,const char* id){ preset_->addItem(name,id); };
    switch (layer->type) {
    case document::CurveType::PolarRose:
        add("Sevenfold garden","rose-seven"); add("Prime lace 11","rose-eleven");
        add("Compass bloom","rose-compass"); add("Solar flower 32","rose-solar");
        add("Decimal k = 2","rose-decimal-2");
        add("Decimal k = 3","rose-decimal-3");
        add("Fraction k = 1/2","rose-fraction-1-2");
        add("Fraction k = 3/2","rose-fraction-3-2");
        add("Fraction k = 5/2","rose-fraction-5-2");
        add("Fraction k = 7/2","rose-fraction-7-2");
        add("Fraction k = 1/3","rose-fraction-1-3");
        add("Fraction k = 2/3","rose-fraction-2-3");
        add("Fraction k = 4/3","rose-fraction-4-3");
        add("Fraction k = 5/3","rose-fraction-5-3");
        add("Fraction k = 7/3","rose-fraction-7-3");
        add("Fraction k = 1/4","rose-fraction-1-4");
        add("Fraction k = 2/4","rose-fraction-2-4");
        add("Fraction k = 3/4","rose-fraction-3-4");
        add("Fraction k = 5/4","rose-fraction-5-4");
        add("Fraction k = 6/4","rose-fraction-6-4");
        add("Fraction k = 7/4","rose-fraction-7-4");
        break;
    case document::CurveType::Ellipse:
        add("Perfect circle","ellipse-circle"); add("Golden ellipse","ellipse-golden");
        add("Tilted medallion","ellipse-tilted"); add("Needle orbit","ellipse-needle"); break;
    case document::CurveType::Hypotrochoid:
        add("Deep sevenfold star","hypo-seven"); add("Nested deltoid","hypo-deltoid");
        add("Rotating astroid","hypo-astroid"); add("Wide stellar web","hypo-web"); break;
    case document::CurveType::Epitrochoid:
        add("Two-turn outer orbit 32/63","epi-orbit"); add("Crowned epicycloid","epi-crown");
        add("Prime outer lace","epi-prime"); add("Wide orbital bloom","epi-bloom"); break;
    case document::CurveType::Lissajous:
        add("Classic 3:2","liss-3-2"); add("Cathedral weave 5:4","liss-5-4");
        add("Prime knot 7:5","liss-7-5"); add("Woven lattice 9:8","liss-9-8"); break;
    case document::CurveType::Harmonograph:
        add("Whispering ellipse","harm-whisper"); add("Decaying flower 5:4","harm-flower");
        add("Dense weave 7:5","harm-weave"); add("Damped butterfly","harm-butterfly");
        add("Long-decay meditation","harm-meditation"); break;
    case document::CurveType::DropletRosette:
        add("Taijitu pair", "droplet-2");
        add("Triple tomoe", "droplet-3");
        add("Fivefold wheel", "droplet-5");
        add("Eightfold vortex", "droplet-8");
        break;
    case document::CurveType::Count: break;
    }
    active_preset_id_=QString::fromStdString(layer->preset_id);
    const int remembered=preset_->findData(active_preset_id_);
    if (remembered>0 && !layer->preset_customized) preset_->setCurrentIndex(remembered);
    else preset_->setCurrentIndex(0);
    restore_preset_button_->setEnabled(remembered>0);
}

void MainWindow::apply_selected_preset()
{
    const auto id=preset_->currentData().toString();
    if (id.isEmpty()) return;
    active_preset_id_=id;
    if (auto* layer=document_.find_layer(active_layer_id_)) {
        layer->preset_id=id.toStdString();
        layer->preset_customized=false;
    }
    applying_preset_=true;
    const bool was_tracking_document_changes = track_document_changes_;
    track_document_changes_ = false;
    if (id.startsWith("rose-decimal-")) {
        const auto value = id.mid(QString("rose-decimal-").size()).toDouble();
        k_mode_->setCurrentIndex(k_mode_->findData(static_cast<int>(curves::PolarKMode::Decimal)));
        radius_->setValue(100); k_->setValue(value);
        numerator_->setValue(static_cast<int>(value)); denominator_->setValue(1);
        phase_->setValue(0); rotation_->setValue(0);
    } else if (id.startsWith("rose-fraction-")) {
        const auto parts = id.mid(QString("rose-fraction-").size()).split('-');
        if (parts.size()==2) {
            const int numerator = parts[0].toInt();
            const int denominator = parts[1].toInt();
            k_mode_->setCurrentIndex(k_mode_->findData(static_cast<int>(curves::PolarKMode::Fraction)));
            radius_->setValue(100);
            k_->setValue(static_cast<double>(numerator)/static_cast<double>(denominator));
            numerator_->setValue(numerator); denominator_->setValue(denominator);
            phase_->setValue(0); rotation_->setValue(0);
        }
    } else if (id=="rose-seven") { k_mode_->setCurrentIndex(0); radius_->setValue(100); k_->setValue(7); phase_->setValue(0); rotation_->setValue(0); }
    else if (id=="rose-eleven") { k_mode_->setCurrentIndex(0); radius_->setValue(100); k_->setValue(11); phase_->setValue(8); rotation_->setValue(0); }
    else if (id=="rose-compass") { k_mode_->setCurrentIndex(0); radius_->setValue(95); k_->setValue(4); phase_->setValue(0); rotation_->setValue(22.5); }
    else if (id=="rose-solar") { k_mode_->setCurrentIndex(0); radius_->setValue(100); k_->setValue(16); phase_->setValue(5.625); rotation_->setValue(0); }
    else if (id=="ellipse-circle") { ellipse_radius_x_->setValue(75); ellipse_link_radii_->setChecked(true); ellipse_rotation_->setValue(0); }
    else if (id=="ellipse-golden") { ellipse_link_radii_->setChecked(false); ellipse_radius_x_->setValue(80); ellipse_radius_y_->setValue(49.443); ellipse_rotation_->setValue(0); }
    else if (id=="ellipse-tilted") { ellipse_link_radii_->setChecked(false); ellipse_radius_x_->setValue(82); ellipse_radius_y_->setValue(48); ellipse_rotation_->setValue(45); }
    else if (id=="ellipse-needle") { ellipse_link_radii_->setChecked(false); ellipse_radius_x_->setValue(100); ellipse_radius_y_->setValue(22); ellipse_rotation_->setValue(30); }
    else if (id.startsWith("hypo-")) {
        trochoid_trace_mode_->setCurrentIndex(trochoid_trace_mode_->findData(static_cast<int>(curves::TraceMode::Complete)));
        if (id=="hypo-seven") { trochoid_fixed_radius_->setValue(105); trochoid_rolling_radius_->setValue(45); trochoid_pen_offset_->setValue(32); }
        else if (id=="hypo-deltoid") { trochoid_fixed_radius_->setValue(90); trochoid_rolling_radius_->setValue(30); trochoid_pen_offset_->setValue(30); }
        else if (id=="hypo-astroid") { trochoid_fixed_radius_->setValue(100); trochoid_rolling_radius_->setValue(25); trochoid_pen_offset_->setValue(25); trochoid_rotation_->setValue(45); }
        else { trochoid_fixed_radius_->setValue(105); trochoid_rolling_radius_->setValue(60); trochoid_pen_offset_->setValue(52); }
    } else if (id.startsWith("epi-")) {
        trochoid_trace_mode_->setCurrentIndex(trochoid_trace_mode_->findData(static_cast<int>(id=="epi-orbit"?curves::TraceMode::Limited:curves::TraceMode::Complete)));
        if (id=="epi-orbit") { trochoid_fixed_radius_->setValue(32); trochoid_rolling_radius_->setValue(63); trochoid_pen_offset_->setValue(44.5); trochoid_turns_->setValue(2); trochoid_close_limited_->setChecked(false); }
        else if (id=="epi-crown") { trochoid_fixed_radius_->setValue(80); trochoid_rolling_radius_->setValue(30); trochoid_pen_offset_->setValue(30); }
        else if (id=="epi-prime") { trochoid_fixed_radius_->setValue(70); trochoid_rolling_radius_->setValue(27); trochoid_pen_offset_->setValue(24); }
        else { trochoid_fixed_radius_->setValue(55); trochoid_rolling_radius_->setValue(34); trochoid_pen_offset_->setValue(45); }
    } else if (id.startsWith("liss-")) {
        lissajous_amplitude_x_->setValue(80); lissajous_amplitude_y_->setValue(80); lissajous_phase_x_->setValue(90);
        if (id=="liss-3-2") { lissajous_frequency_x_->setValue(3); lissajous_frequency_y_->setValue(2); lissajous_phase_y_->setValue(0); }
        else if (id=="liss-5-4") { lissajous_frequency_x_->setValue(5); lissajous_frequency_y_->setValue(4); lissajous_phase_y_->setValue(117); }
        else if (id=="liss-7-5") { lissajous_frequency_x_->setValue(7); lissajous_frequency_y_->setValue(5); lissajous_phase_y_->setValue(38.571); }
        else { lissajous_frequency_x_->setValue(9); lissajous_frequency_y_->setValue(8); lissajous_phase_y_->setValue(80); }
    } else if (id.startsWith("harm-")) {
        harmonograph_amplitude_x_->setValue(80); harmonograph_amplitude_y_->setValue(80); harmonograph_rotation_->setValue(0);
        if (id=="harm-whisper") { harmonograph_frequency_x_->setValue(1); harmonograph_frequency_y_->setValue(1.005); harmonograph_phase_x_->setValue(0); harmonograph_phase_y_->setValue(90); harmonograph_damping_x_->setValue(.01); harmonograph_damping_y_->setValue(.012); harmonograph_duration_->setValue(140); }
        else if (id=="harm-flower") { harmonograph_frequency_x_->setValue(5); harmonograph_frequency_y_->setValue(4.03); harmonograph_phase_x_->setValue(90); harmonograph_phase_y_->setValue(0); harmonograph_damping_x_->setValue(.018); harmonograph_damping_y_->setValue(.012); harmonograph_duration_->setValue(45); }
        else if (id=="harm-weave") { harmonograph_frequency_x_->setValue(7); harmonograph_frequency_y_->setValue(5.02); harmonograph_phase_x_->setValue(90); harmonograph_phase_y_->setValue(12); harmonograph_damping_x_->setValue(.012); harmonograph_damping_y_->setValue(.009); harmonograph_duration_->setValue(55); }
        else if (id=="harm-butterfly") { harmonograph_frequency_x_->setValue(2); harmonograph_frequency_y_->setValue(1.01); harmonograph_phase_x_->setValue(0); harmonograph_phase_y_->setValue(90); harmonograph_damping_x_->setValue(.025); harmonograph_damping_y_->setValue(.01); harmonograph_duration_->setValue(90); }
        else { harmonograph_frequency_x_->setValue(1); harmonograph_frequency_y_->setValue(1.003); harmonograph_phase_x_->setValue(90); harmonograph_phase_y_->setValue(0); harmonograph_damping_x_->setValue(.003); harmonograph_damping_y_->setValue(.004); harmonograph_duration_->setValue(300); }
    } else if (id.startsWith("droplet-")) {
        droplet_count_->setValue(id.mid(QString("droplet-").size()).toInt());
        droplet_outer_radius_->setValue(80);
        droplet_core_radius_->setValue(id == "droplet-2" ? 0 : 16);
        droplet_swirl_->setValue(id == "droplet-2" ? 42 : 28);
        droplet_width_->setValue(id == "droplet-8" ? 82 : 88);
        droplet_roundness_->setValue(0.55);
        droplet_rotation_->setValue(-90);
    }
    restore_preset_button_->setEnabled(true);
    update_preview();
    track_document_changes_ = was_tracking_document_changes;
    applying_preset_=false;
    mark_document_modified();
}

void MainWindow::restore_active_preset()
{
    if (active_preset_id_.isEmpty()) return;
    const int index=preset_->findData(active_preset_id_);
    if (index<0) return;
    { const QSignalBlocker blocker(preset_); preset_->setCurrentIndex(index); }
    apply_selected_preset();
}

} // namespace rosettelab::ui
