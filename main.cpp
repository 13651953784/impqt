#include "impqt.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ImpQt w;
    w.show();
    return a.exec();
}
