#include "mainwindow.h"
#include "fileview.h"
#include "sidebar.h"
#include "drivespage.h"
#include "browsepage.h"
#include "playerpage.h"
#include "settingspage.h"

#include <QHBoxLayout>
#include <QStackedWidget> 
#include <QLabel>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Kino");
    resize(1200, 720);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0); 
    layout->setSpacing(0);
    central->setStyleSheet("background-color: #1a1b26;");

    Sidebar *sidebar = new Sidebar(this);
    layout->addWidget(sidebar);

    QStackedWidget *stack = new QStackedWidget(this);
    layout->addWidget(stack, 1); 

    QLabel *startupLoader = new QLabel("Initializing Engine...", this);
    startupLoader->setStyleSheet("color: #565f89; font-size: 18px; font-weight: bold;");
    startupLoader->setAlignment(Qt::AlignCenter);
    stack->addWidget(startupLoader);
    stack->setCurrentWidget(startupLoader);

    QTimer::singleShot(50, this, [this, sidebar, stack, startupLoader]() {
        
        FileView *library = new FileView(this);
        stack->addWidget(library); 

        BrowsePage *browse = new BrowsePage(this);
        stack->addWidget(browse);

        DrivesPage *drives = new DrivesPage(this);
        stack->addWidget(drives);

        SettingsPage *settingsPage = new SettingsPage(this);
        stack->addWidget(settingsPage); 

        PlayerPage *player = new PlayerPage(this);
        stack->addWidget(player);


        connect(drives, &DrivesPage::mountsChanged, [drives, library, browse](){
            library->setMounts(drives->getActiveMountPaths());
            browse->reload();
        });

        connect(drives, &DrivesPage::scanRequested, [library, stack](const QString &path){
            library->setMounts(QStringList() << path);
            library->startScan();
            stack->setCurrentWidget(library); 
        });

        connect(sidebar, &Sidebar::directorySelected, [=](const QString &command){
            player->stop();
            if (command == ":drives") {
                drives->refresh(); 
                stack->setCurrentWidget(drives);
            } else if (command == ":browse") {
                browse->refresh(); 
                stack->setCurrentWidget(browse);
            } else if (command == ":settings") {
                stack->setCurrentWidget(settingsPage);
            } else {
                stack->setCurrentWidget(library);
            }
        });

        connect(library, &FileView::playVideoRequested, [=](const QString &path, const QString &title, const QPixmap &backdrop){
            player->setProperty("returnPage", "library"); 
            sidebar->hide(); 
            stack->setCurrentWidget(player); 
            player->play(path, title, backdrop);
        });

        connect(player, &PlayerPage::backRequested, [=](){
            sidebar->show();
            if (player->property("returnPage").toString() == "browse") {
                stack->setCurrentWidget(browse);
            } else {
                stack->setCurrentWidget(library);
            }
        });

        connect(settingsPage, &SettingsPage::tmdbKeyUpdated, library, &FileView::refreshMetadata);

        connect(browse, &BrowsePage::playVideoRequested, [=](const QString &path, const QString &title, const QPixmap &backdrop){
            player->setProperty("returnPage", "browse"); 
            sidebar->hide(); 
            stack->setCurrentWidget(player); 
            player->play(path, title, backdrop);
        });

        stack->setCurrentWidget(library);
        startupLoader->deleteLater();
    });
}