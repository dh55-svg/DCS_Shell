#include <QApplication>
#include <QDebug>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qDebug() << "DCS_Shell starting...";
    return app.exec();
}
