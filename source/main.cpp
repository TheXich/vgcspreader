#include <QApplication>
#include <QCoreApplication>
#include "mainwindow.hpp"

int main( int argc, char **argv ) {
    QApplication app(argc, argv);

    //identifies the per-user data folder where presets and saved calcs live (see MainWindow::userDataFilePath)
    QCoreApplication::setApplicationName("VGCSpreader");

    MainWindow main_win;

    main_win.show();
    return app.exec();
}
