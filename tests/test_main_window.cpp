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
    return 0;
}
