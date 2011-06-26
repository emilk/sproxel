#include "MainWindow.h"
#include "script.h"

#include <QtGui>
#include <QApplication>

int main(int argc, char *argv[])
{
    QString filename = "";
    if (argc > 1)
        filename = argv[1];

    QApplication a(argc, argv);

    init_script(argc>=1 ? argv[0] : "sproxel.exe");

    MainWindow window(filename);
    window.show();

    script_set_main_window(&window);
    run_script("test.py");

    int r=a.exec();
    close_script();
    return r;
}
