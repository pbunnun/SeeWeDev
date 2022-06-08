#include "MainWindow.hpp"

#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>

int main(int argc, char *argv[])
{
    qDebug() <<"What!";

    QApplication app(argc, argv);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    app.setWindowIcon(QIcon(":/cvdev-64.png"));

    MainWindow window;
    window.show();
    return app.exec();
}
