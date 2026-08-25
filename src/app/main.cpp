#include "ui/main_window.hpp"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("RosetteLab");
    application.setOrganizationName("RosetteLab");

    rosettelab::ui::MainWindow window;
    window.show();
    return application.exec();
}

