#include "ui/main_window.hpp"

#include "ui/preview_widget.hpp"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

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

    samples_ = new QSpinBox(curve_group);
    samples_->setRange(16, 250000);
    samples_->setValue(720);

    form->addRow("Radius a", radius_);
    form->addRow("Parameter k", k_);
    form->addRow("Phase", phase_);
    form->addRow("Rotation", rotation_);
    form->addRow("Samples", samples_);
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

    preview_ = new PreviewWidget(splitter);

    auto* layers_panel = new QWidget(splitter);
    auto* layers_layout = new QVBoxLayout(layers_panel);
    layers_layout->addWidget(new QLabel("Layers", layers_panel));
    auto* layers = new QListWidget(layers_panel);
    auto* initial_layer = new QListWidgetItem("Polar rose - k=7", layers);
    initial_layer->setFlags(initial_layer->flags() | Qt::ItemIsUserCheckable);
    initial_layer->setCheckState(Qt::Checked);
    layers_layout->addWidget(layers);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({280, 640, 280});

    connect(radius_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview(); });
    connect(k_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview(); });
    connect(phase_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview(); });
    connect(rotation_, &QDoubleSpinBox::valueChanged, this, [this] { update_preview(); });
    connect(samples_, &QSpinBox::valueChanged, this, [this] { update_preview(); });
    connect(zoom_, &QSpinBox::valueChanged, this, [this](const int value) {
        preview_->set_zoom_percent(static_cast<double>(value));
    });

    update_preview();
}

void MainWindow::update_preview()
{
    curves::PolarRoseParameters parameters;
    parameters.radius = radius_->value();
    parameters.k = k_->value();
    parameters.phase_degrees = phase_->value();
    parameters.rotation_degrees = rotation_->value();
    parameters.samples = static_cast<std::size_t>(samples_->value());
    preview_->set_parameters(parameters);
}

} // namespace rosettelab::ui
