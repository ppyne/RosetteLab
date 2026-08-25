#include "ui/main_window.hpp"

#include "ui/layer_list_item_widget.hpp"
#include "ui/preview_widget.hpp"

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace rosettelab::ui {
namespace {

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

void style_color_button(QPushButton* button, const QColor& color)
{
    button->setText(QStringLiteral("#%1%2%3%4")
        .arg(color.red(), 2, 16, QLatin1Char('0'))
        .arg(color.green(), 2, 16, QLatin1Char('0'))
        .arg(color.blue(), 2, 16, QLatin1Char('0'))
        .arg(color.alpha(), 2, 16, QLatin1Char('0'))
        .toUpper());
    const auto text = color.lightnessF() < 0.5 ? QStringLiteral("white") : QStringLiteral("black");
    button->setStyleSheet(QStringLiteral("background-color: rgba(%1, %2, %3, %4); color: %5;")
        .arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha()).arg(text));
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("RosetteLab");
    resize(1200, 760);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    auto* parameters_panel = new QWidget(splitter);
    auto* parameters_layout = new QVBoxLayout(parameters_panel);
    parameters_layout->addWidget(new QLabel("Polar rose", parameters_panel));

    curve_group_ = new QGroupBox("Curve parameters", parameters_panel);
    auto* form = new QFormLayout(curve_group_);

    radius_ = new QDoubleSpinBox(curve_group_);
    radius_->setRange(0.01, 100000.0);
    radius_->setValue(100.0);
    radius_->setDecimals(2);

    k_ = new QDoubleSpinBox(curve_group_);
    k_->setRange(-1000.0, 1000.0);
    k_->setValue(7.0);
    k_->setDecimals(3);

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
    form->addRow("Parameter k", k_);
    form->addRow("Phase", phase_);
    form->addRow("Rotation", rotation_);
    form->addRow("Curve tolerance", tolerance_);
    parameters_layout->addWidget(curve_group_);

    appearance_group_ = new QGroupBox("Appearance", parameters_panel);
    auto* appearance_form = new QFormLayout(appearance_group_);
    stroke_color_button_ = new QPushButton(appearance_group_);
    fill_color_button_ = new QPushButton(appearance_group_);

    stroke_width_ = new QDoubleSpinBox(appearance_group_);
    stroke_width_->setRange(0.0, 1000.0);
    stroke_width_->setValue(0.6);
    stroke_width_->setDecimals(2);
    stroke_width_->setSuffix(" units");

    fill_enabled_ = new QCheckBox("Enabled", appearance_group_);
    fill_rule_ = new QComboBox(appearance_group_);
    fill_rule_->addItem("Non-zero", static_cast<int>(document::FillRule::NonZero));
    fill_rule_->addItem("Even-odd", static_cast<int>(document::FillRule::EvenOdd));

    layer_opacity_ = new QSpinBox(appearance_group_);
    layer_opacity_->setRange(0, 100);
    layer_opacity_->setValue(100);
    layer_opacity_->setSuffix(" %");

    blend_mode_ = new QComboBox(appearance_group_);
    const std::initializer_list<std::pair<const char*, document::BlendMode>> blend_modes{
        {"Normal", document::BlendMode::Normal}, {"Multiply", document::BlendMode::Multiply},
        {"Screen", document::BlendMode::Screen}, {"Overlay", document::BlendMode::Overlay},
        {"Darken", document::BlendMode::Darken}, {"Lighten", document::BlendMode::Lighten},
        {"Color dodge", document::BlendMode::ColorDodge}, {"Color burn", document::BlendMode::ColorBurn},
        {"Hard light", document::BlendMode::HardLight}, {"Soft light", document::BlendMode::SoftLight},
        {"Difference", document::BlendMode::Difference}, {"Exclusion", document::BlendMode::Exclusion},
    };
    for (const auto& [name, mode] : blend_modes) {
        blend_mode_->addItem(name, static_cast<int>(mode));
    }

    appearance_form->addRow("Stroke color", stroke_color_button_);
    appearance_form->addRow("Stroke width", stroke_width_);
    appearance_form->addRow("Fill", fill_enabled_);
    appearance_form->addRow("Fill color", fill_color_button_);
    appearance_form->addRow("Fill rule", fill_rule_);
    appearance_form->addRow("Layer opacity", layer_opacity_);
    appearance_form->addRow("Blend mode", blend_mode_);
    parameters_layout->addWidget(appearance_group_);
    refresh_color_buttons();

    auto* view_group = new QGroupBox("View", parameters_panel);
    auto* view_form = new QFormLayout(view_group);
    zoom_ = new QSpinBox(view_group);
    zoom_->setRange(10, 800);
    zoom_->setValue(100);
    zoom_->setSuffix(" %");
    view_form->addRow("Zoom", zoom_);
    parameters_layout->addWidget(view_group);
    parameters_layout->addStretch();

    auto* preview_scroll = new QScrollArea(splitter);
    preview_scroll->setWidgetResizable(false);
    preview_scroll->setAlignment(Qt::AlignCenter);
    preview_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    preview_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    preview_ = new PreviewWidget;
    preview_->set_document(&document_);
    preview_scroll->setWidget(preview_);

    auto* layers_panel = new QWidget(splitter);
    auto* layers_layout = new QVBoxLayout(layers_panel);
    layers_layout->addWidget(new QLabel("Layers", layers_panel));
    layers_ = new QListWidget(layers_panel);
    layers_->setDragDropMode(QAbstractItemView::InternalMove);
    auto& initial = document_.add_polar_rose();
    active_layer_id_ = initial.id;
    auto* initial_layer = add_layer_row(initial);
    layers_->setCurrentItem(initial_layer);
    layers_layout->addWidget(layers_);
    auto* add_button = new QPushButton("Add...", layers_panel);
    auto* add_menu = new QMenu(add_button);
    add_menu->addAction("Polar rose", this, [this] { add_polar_rose(); });
    for (const auto* unavailable : {
             "Hypotrochoid", "Epitrochoid", "Lissajous", "Harmonograph", "Spirograph"}) {
        add_menu->addAction(unavailable)->setEnabled(false);
    }
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

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({280, 640, 280});

    connect(radius_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview(); });
    connect(k_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview(); });
    connect(phase_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview(); });
    connect(rotation_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview(); });
    connect(tolerance_, &QDoubleSpinBox::valueChanged, this, [this](const double value) {
        preview_->set_curve_tolerance(value);
    });
    connect(stroke_color_button_, &QPushButton::clicked, this, [this] { choose_stroke_color(); });
    connect(fill_color_button_, &QPushButton::clicked, this, [this] { choose_fill_color(); });
    connect(stroke_width_, &QDoubleSpinBox::valueChanged, this, [this] { update_appearance(); });
    connect(fill_enabled_, &QCheckBox::toggled, this, [this] { update_appearance(); });
    connect(fill_rule_, &QComboBox::currentIndexChanged, this, [this] { update_appearance(); });
    connect(layer_opacity_, &QSpinBox::valueChanged, this, [this] { update_appearance(); });
    connect(blend_mode_, &QComboBox::currentIndexChanged, this, [this] { update_appearance(); });
    connect(zoom_, &QSpinBox::valueChanged, this, [this](const int value) {
        preview_->set_zoom_percent(static_cast<double>(value));
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

    update_preview();
    refresh_layer_actions();
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
    auto* item = add_layer_row(layer);
    layers_->setCurrentItem(item);
    preview_->update();
}

QListWidgetItem* MainWindow::add_layer_row(const document::CurveLayer& layer, const int row)
{
    auto* item = new QListWidgetItem;
    item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(layer.id));
    item->setSizeHint(QSize(220, 32));
    if (row < 0) {
        layers_->addItem(item);
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
        },
        [this, id = layer.id](const bool locked) {
            select_layer_row(id);
            set_active_layer_locked(locked);
        });
    layers_->setItemWidget(item, widget);
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
    const auto* parameters = std::get_if<curves::PolarRoseParameters>(&layer->parameters);
    if (parameters == nullptr) {
        return;
    }

    const QSignalBlocker radius_blocker(radius_);
    const QSignalBlocker k_blocker(k_);
    const QSignalBlocker phase_blocker(phase_);
    const QSignalBlocker rotation_blocker(rotation_);
    const QSignalBlocker stroke_width_blocker(stroke_width_);
    const QSignalBlocker fill_enabled_blocker(fill_enabled_);
    const QSignalBlocker fill_rule_blocker(fill_rule_);
    const QSignalBlocker opacity_blocker(layer_opacity_);
    const QSignalBlocker blend_blocker(blend_mode_);
    radius_->setValue(parameters->radius);
    k_->setValue(parameters->k);
    phase_->setValue(parameters->phase_degrees);
    rotation_->setValue(parameters->rotation_degrees);
    stroke_color_ = qcolor_from_rgba(layer->appearance.stroke);
    fill_color_ = qcolor_from_rgba(layer->appearance.fill);
    stroke_width_->setValue(layer->appearance.stroke_width);
    fill_enabled_->setChecked(layer->appearance.fill_enabled);
    fill_rule_->setCurrentIndex(fill_rule_->findData(static_cast<int>(layer->appearance.fill_rule)));
    layer_opacity_->setValue(static_cast<int>(std::lround(layer->appearance.opacity * 100.0)));
    blend_mode_->setCurrentIndex(blend_mode_->findData(static_cast<int>(layer->appearance.blend_mode)));
    refresh_color_buttons();
}

void MainWindow::choose_stroke_color()
{
    const auto color = QColorDialog::getColor(
        stroke_color_, this, "Stroke color", QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        stroke_color_ = color;
        refresh_color_buttons();
        update_appearance();
    }
}

void MainWindow::choose_fill_color()
{
    const auto color = QColorDialog::getColor(
        fill_color_, this, "Fill color", QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        fill_color_ = color;
        refresh_color_buttons();
        update_appearance();
    }
}

void MainWindow::update_appearance()
{
    auto* layer = document_.find_layer(active_layer_id_);
    if (layer == nullptr || layer->locked) {
        return;
    }
    layer->appearance.stroke = rgba_from_qcolor(stroke_color_);
    layer->appearance.stroke_width = stroke_width_->value();
    layer->appearance.fill_enabled = fill_enabled_->isChecked();
    layer->appearance.fill = rgba_from_qcolor(fill_color_);
    layer->appearance.fill_rule = static_cast<document::FillRule>(fill_rule_->currentData().toInt());
    layer->appearance.opacity = static_cast<double>(layer_opacity_->value()) / 100.0;
    layer->appearance.blend_mode = static_cast<document::BlendMode>(blend_mode_->currentData().toInt());
    refresh_color_buttons();
    preview_->update();
}

void MainWindow::refresh_color_buttons()
{
    style_color_button(stroke_color_button_, stroke_color_);
    style_color_button(fill_color_button_, fill_color_);
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
    }
}

void MainWindow::duplicate_active_layer()
{
    const auto source_row = layers_->currentRow();
    auto* duplicate = document_.duplicate_layer(active_layer_id_);
    if (duplicate == nullptr) {
        return;
    }

    auto* item = add_layer_row(*duplicate, source_row + 1);
    layers_->setCurrentItem(item);
    preview_->update();
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
}

void MainWindow::set_active_layer_locked(const bool locked)
{
    if (document_.set_layer_locked(active_layer_id_, locked)) {
        refresh_layer_actions();
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
    appearance_group_->setEnabled(has_layer && !layer->locked);
}

void MainWindow::sync_layer_order()
{
    for (int target = 0; target < layers_->count(); ++target) {
        const auto id = static_cast<document::LayerId>(
            layers_->item(target)->data(Qt::UserRole).toULongLong());
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
}

void MainWindow::update_preview()
{
    auto* layer = document_.find_layer(active_layer_id_);
    if (layer == nullptr || layer->locked) {
        return;
    }
    auto* parameters = std::get_if<curves::PolarRoseParameters>(&layer->parameters);
    if (parameters == nullptr) {
        return;
    }
    parameters->radius = radius_->value();
    parameters->k = k_->value();
    parameters->phase_degrees = phase_->value();
    parameters->rotation_degrees = rotation_->value();
    preview_->update();
}

} // namespace rosettelab::ui
