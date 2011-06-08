#include "MainWindow.h"

#include <QtGui>
#include <QApplication>

int main(int argc, char *argv[])
{
    QString filename = "";
    if (argc > 1)
        filename = argv[1];

    QApplication a(argc, argv);
    MainWindow window(filename);
    window.show();
    return a.exec();
}
