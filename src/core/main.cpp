#include <QApplication>
#include <QFontDatabase>
#include <QFont>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QVBoxLayout>
#include <QEvent>
#include <QIcon>
#include <clocale>
#include "mainwindow.h"
#include <QPointer>
class SplashFilter : public QObject {
public:
    QPointer<QWidget> overlay; 
    SplashFilter(QWidget *o, QObject *parent = nullptr) : QObject(parent), overlay(o) {}
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Resize && overlay) {
            overlay->setGeometry(static_cast<QWidget*>(obj)->rect());
        }
        return QObject::eventFilter(obj, event);
    }
};

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "xcb");
    QApplication app(argc, argv);
    setlocale(LC_NUMERIC, "C");
    app.setApplicationName("Kino");
    app.setApplicationDisplayName("Kino");
    app.setDesktopFileName("Kino");
    QString fontFamily = "Sans Serif";
    int fontId = QFontDatabase::addApplicationFont(":/fonts/OpenSans-VariableFont.ttf");
    if (fontId != -1) {
        fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
        QFont appFont(fontFamily);
        app.setFont(appFont);
    }

    MainWindow window;

    QWidget *splash = new QWidget(&window);
    splash->setStyleSheet("background-color: #1a1b26;");
    
    QVBoxLayout *lay = new QVBoxLayout(splash);
    lay->setAlignment(Qt::AlignCenter);
    
    QLabel *logoLabel = new QLabel(splash);
    logoLabel->setPixmap(QIcon(":/icons/logo.svg").pixmap(120, 120));
    logoLabel->setAlignment(Qt::AlignCenter);
    lay->addWidget(logoLabel);

    QLabel *textLabel = new QLabel("KINO", splash);
    textLabel->setStyleSheet("color: #c0caf5; background: transparent;");
    textLabel->setFont(QFont(fontFamily, 36, QFont::ExtraBold));
    textLabel->setAlignment(Qt::AlignCenter);
    lay->addWidget(textLabel);

    window.installEventFilter(new SplashFilter(splash, &window));

    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(splash);
    splash->setGraphicsEffect(eff);

    window.show();
    splash->setGeometry(window.rect());
    splash->raise();

    QTimer::singleShot(1500, [splash, eff]() {
        QPropertyAnimation *anim = new QPropertyAnimation(eff, "opacity");
        anim->setDuration(500);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        QObject::connect(anim, &QPropertyAnimation::finished, splash, &QObject::deleteLater);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });
    
    return app.exec();
}