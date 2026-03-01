#include <QApplication>
#include <QFontDatabase>
#include <QFont>
#include "mainwindow.h"
#include <clocale>

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "xcb");
    QApplication app(argc, argv);
    setlocale(LC_NUMERIC, "C");

    int fontId = QFontDatabase::addApplicationFont(":/fonts/OpenSans-VariableFont.ttf");
    if (fontId != -1) {
        QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
        QFont appFont(fontFamily);
        
        
        app.setFont(appFont);
    }

    MainWindow window;
    window.show();
    
    return app.exec();
}