#include "ui/main_window.hpp"

#include "ui/preview_widget.hpp"

#include <QAbstractItemModel>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

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

    auto* curve_group = new QGroupBox("Curve parameters", parameters_panel);
    auto* form = new QFormLayout(curve_group);

    radius_ = new QDoubleSpinBox(curve_group);
    radius_->setRange(0.01, 100000.0);
    radius_->setValue(100.0);
    radius_->setDecimals(2);

    k_ = new QDoubleSpinBox(curve_group);
    k_->setRange(-1000.0, 1000.0);
    k_->setValue(7.0);
    k_->setDecimals(3);

    phase_ = angle_control(curve_group);
    rotation_ = angle_control(curve_group);

    tolerance_ = new QDoubleSpinBox(curve_group);
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
    parameters_layout->addWidget(curve_group);

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
    auto* initial_layer = new QListWidgetItem(QString::fromStdString(initial.name), layers_);
    initial_layer->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(initial.id));
    initial_layer->setFlags(initial_layer->flags() | Qt::ItemIsUserCheckable);
    initial_layer->setCheckState(Qt::Checked);
    layers_->setCurrentItem(initial_layer);
    layers_layout->addWidget(layers_);
    auto* add_button = new QPushButton("Add polar rose", layers_panel);
    layers_layout->addWidget(add_button);

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
    connect(zoom_, &QSpinBox::valueChanged, this, [this](const int value) {
        preview_->set_zoom_percent(static_cast<double>(value));
    });
    connect(add_button, &QPushButton::clicked, this, [this] { add_polar_rose(); });
    connect(layers_, &QListWidget::currentItemChanged, this,
        [this](QListWidgetItem* current, QListWidgetItem*) {
            if (current != nullptr) {
                select_layer(current->data(Qt::UserRole).toULongLong());
            }
        });
    connect(layers_, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        const auto id = static_cast<document::LayerId>(item->data(Qt::UserRole).toULongLong());
        static_cast<void>(document_.set_layer_visible(id, item->checkState() == Qt::Checked));
        preview_->update();
    });
    connect(layers_->model(), &QAbstractItemModel::rowsMoved, this,
        [this] { sync_layer_order(); });

    update_preview();
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
    auto* item = new QListWidgetItem(name, layers_);
    item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(layer.id));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Checked);
    layers_->setCurrentItem(item);
    preview_->update();
}

void MainWindow::select_layer(const document::LayerId id)
{
    active_layer_id_ = id;
    load_active_layer();
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
    radius_->setValue(parameters->radius);
    k_->setValue(parameters->k);
    phase_->setValue(parameters->phase_degrees);
    rotation_->setValue(parameters->rotation_degrees);
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
