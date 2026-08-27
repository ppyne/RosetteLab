#include "ui/main_window.hpp"

#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("RosetteLab");
    application.setApplicationDisplayName("RosetteLab");
    application.setOrganizationName("RosetteLab");
    application.setOrganizationDomain("rosettelab.org");
    application.setApplicationVersion(ROSETTELAB_VERSION);
    application.setWindowIcon(QIcon(":/icons/rosettelab.png"));

    rosettelab::ui::MainWindow window;
    window.show();
    return application.exec();
}
