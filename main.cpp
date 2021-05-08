#include "injector.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Injector w;
    w.setWindowTitle("Python Injector");
    w.show();
    return a.exec();
}
