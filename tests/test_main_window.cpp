#include "ui/main_window.hpp"

#include <QApplication>
#include <QComboBox>

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
    return 0;
}
