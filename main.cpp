#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow main_window;
    QStringList args = app.arguments();
    if(args.size() > 1)
    {
        args.removeFirst();
        main_window.openFiles(args);
    }
    main_window.show();
    return app.exec();
}
