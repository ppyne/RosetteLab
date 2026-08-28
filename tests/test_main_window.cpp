#include "ui/main_window.hpp"

#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSpinBox>

#include <iostream>

int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    rosettelab::ui::MainWindow window;
    auto* selector=window.findChild<QComboBox*>("presetSelector");
    if (selector==nullptr || selector->count()<2 ||
        selector->currentData().toString()!="rose-seven" ||
        selector->currentText()!="Sevenfold garden") {
        std::cerr << "Initial Polar rose layer did not select Sevenfold garden\n";
        return 1;
    }
    const auto* zoom_levels=window.findChild<QComboBox*>("zoomLevelSelector");
    if (zoom_levels==nullptr || zoom_levels->currentData().toDouble()>=0.0 ||
        zoom_levels->findData(0.10)<0 || zoom_levels->findData(3200.00)<0) {
        std::cerr << "Zoom levels or default fit mode were not initialized\n";
        return 1;
    }
    const auto* blend_modes=window.findChild<QComboBox*>("blendModeSelector");
    if (blend_modes==nullptr) {
        std::cerr << "Blend mode selector was not initialized\n";
        return 1;
    }
    constexpr rosettelab::document::BlendMode disabled_blend_modes[] = {
        rosettelab::document::BlendMode::Hue,
        rosettelab::document::BlendMode::Saturation,
        rosettelab::document::BlendMode::Color,
        rosettelab::document::BlendMode::Luminosity,
    };
    for (const auto mode : disabled_blend_modes) {
        const int index=blend_modes->findData(static_cast<int>(mode));
        if (index<0 || (blend_modes->model()->flags(blend_modes->model()->index(index, 0))
                        & Qt::ItemIsEnabled)) {
            std::cerr << "Unsupported preview blend mode was not disabled\n";
            return 1;
        }
    }

    auto* undo=window.findChild<QAction*>("undoAction");
    auto* redo=window.findChild<QAction*>("redoAction");
    auto* transform_x=window.findChild<QDoubleSpinBox*>("transformXField");
    auto* transform_y=window.findChild<QDoubleSpinBox*>("transformYField");
    auto* reset_transform=window.findChild<QPushButton*>("resetTransformButton");
    auto* copy_count=window.findChild<QSpinBox*>("copyCountField");
    auto* reset_copies=window.findChild<QPushButton*>("resetCopiesButton");
    auto* copy_arrangement=window.findChild<QComboBox*>("copyArrangementSelector");
    auto* circular_angle=window.findChild<QDoubleSpinBox*>("copyCircularAngleField");
    auto* distribute_copies=window.findChild<QPushButton*>("distributeCopiesButton");
    auto* polar_k=window.findChild<QDoubleSpinBox*>("polarKField");
    auto* polar_k_mode=window.findChild<QComboBox*>("polarKModeSelector");
    auto* polar_numerator=window.findChild<QSpinBox*>("polarNumeratorField");
    auto* polar_denominator=window.findChild<QSpinBox*>("polarDenominatorField");
    auto* restore_preset=window.findChild<QPushButton*>("restorePresetButton");
    if (undo==nullptr || redo==nullptr || transform_x==nullptr || transform_y==nullptr ||
        reset_transform==nullptr ||
        copy_count==nullptr || reset_copies==nullptr || copy_arrangement==nullptr ||
        circular_angle==nullptr || distribute_copies==nullptr || polar_k==nullptr ||
        polar_k_mode==nullptr || polar_numerator==nullptr || polar_denominator==nullptr ||
        restore_preset==nullptr) {
        std::cerr << "Undo/Redo test controls were not initialized\n";
        return 1;
    }

    transform_x->setValue(10.0);
    transform_x->setValue(20.0);
    transform_x->setValue(25.0);
    undo->trigger();
    if (transform_x->value()!=0.0) {
        std::cerr << "Layer transform edit was not undone\n";
        return 1;
    }
    redo->trigger();
    if (transform_x->value()!=25.0) {
        std::cerr << "Layer transform edit was not redone\n";
        return 1;
    }
    transform_x->setValue(30.0);
    transform_y->setValue(15.0);
    undo->trigger();
    if (transform_x->value()!=30.0 || transform_y->value()!=0.0) {
        std::cerr << "Changes to distinct transform properties were incorrectly coalesced\n";
        return 1;
    }
    undo->trigger();
    if (transform_x->value()!=25.0) {
        std::cerr << "Continuous transform series did not retain its preceding history state\n";
        return 1;
    }
    reset_transform->click();
    undo->trigger();
    if (transform_x->value()!=25.0) {
        std::cerr << "Reset transform was not undone as one operation\n";
        return 1;
    }

    copy_count->setValue(5);
    reset_copies->click();
    undo->trigger();
    if (copy_count->value()!=5) {
        std::cerr << "Reset copies was not undone as one operation\n";
        return 1;
    }
    copy_arrangement->setCurrentIndex(copy_arrangement->findData(
        static_cast<int>(rosettelab::document::CopyArrangement::Circular)));
    copy_count->setValue(12);
    distribute_copies->click();
    if (circular_angle->value()!=30.0) {
        std::cerr << "Circular copies were not distributed over 360 degrees\n";
        return 1;
    }
    undo->trigger();
    if (circular_angle->value()!=0.0) {
        std::cerr << "Circular distribution was not undone as one operation\n";
        return 1;
    }

    polar_k->setValue(9.0);
    restore_preset->click();
    if (polar_k->value()!=7.0 || selector->currentData().toString()!="rose-seven") {
        std::cerr << "Restore preset did not restore Sevenfold garden\n";
        return 1;
    }
    undo->trigger();
    if (polar_k->value()!=9.0 || !selector->currentData().toString().isEmpty()) {
        std::cerr << "Restore preset was not undone as one operation\n";
        return 1;
    }
    redo->trigger();
    if (polar_k->value()!=7.0 || selector->currentData().toString()!="rose-seven") {
        std::cerr << "Restore preset was not redone as one operation\n";
        return 1;
    }

    constexpr const char* requested_polar_presets[] = {
        "rose-decimal-2", "rose-decimal-3",
        "rose-fraction-1-2", "rose-fraction-3-2", "rose-fraction-5-2", "rose-fraction-7-2",
        "rose-fraction-1-3", "rose-fraction-2-3", "rose-fraction-4-3", "rose-fraction-5-3", "rose-fraction-7-3",
        "rose-fraction-1-4", "rose-fraction-2-4", "rose-fraction-3-4", "rose-fraction-5-4", "rose-fraction-6-4", "rose-fraction-7-4",
    };
    for (const auto* preset_id : requested_polar_presets) {
        if (selector->findData(preset_id)<0) {
            std::cerr << "Requested Polar rose preset was not registered: " << preset_id << '\n';
            return 1;
        }
    }
    const int fraction_preset=selector->findData("rose-fraction-7-4");
    selector->setCurrentIndex(fraction_preset);
    if (polar_k_mode->currentData().toInt()!=
            static_cast<int>(rosettelab::curves::PolarKMode::Fraction) ||
        polar_numerator->value()!=7 || polar_denominator->value()!=4 ||
        polar_k->value()!=1.75) {
        std::cerr << "Fraction k = 7/4 preset did not populate its fields\n";
        return 1;
    }
    return 0;
}
