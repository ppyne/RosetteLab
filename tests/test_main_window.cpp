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
    const auto* selector=window.findChild<QComboBox*>("presetSelector");
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

    auto* undo=window.findChild<QAction*>("undoAction");
    auto* redo=window.findChild<QAction*>("redoAction");
    auto* transform_x=window.findChild<QDoubleSpinBox*>("transformXField");
    auto* reset_transform=window.findChild<QPushButton*>("resetTransformButton");
    auto* copy_count=window.findChild<QSpinBox*>("copyCountField");
    auto* reset_copies=window.findChild<QPushButton*>("resetCopiesButton");
    auto* polar_k=window.findChild<QDoubleSpinBox*>("polarKField");
    auto* restore_preset=window.findChild<QPushButton*>("restorePresetButton");
    if (undo==nullptr || redo==nullptr || transform_x==nullptr || reset_transform==nullptr ||
        copy_count==nullptr || reset_copies==nullptr || polar_k==nullptr ||
        restore_preset==nullptr) {
        std::cerr << "Undo/Redo test controls were not initialized\n";
        return 1;
    }

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
    return 0;
}
