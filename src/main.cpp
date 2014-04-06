#include <QApplication>
#include "wafmainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WAFMainWindow w;

    w.show();
    return a.exec();
}
