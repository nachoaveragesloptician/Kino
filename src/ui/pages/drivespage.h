#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QProcess>
#include <QMap>
#include <QPushButton>

struct DriveState {
    bool isMounted = false;
    QProcess *process = nullptr;
    QString mountPath;
    QPushButton *btn = nullptr;
};

class DrivesPage : public QWidget
{
    Q_OBJECT
public:
    explicit DrivesPage(QWidget *parent = nullptr);
    ~DrivesPage();
    void refresh();
    QStringList getActiveMountPaths() const;

signals:
    void mountsChanged();
    void scanRequested(const QString &mountPath);

private slots:
    void toggleMount(const QString &remote);

private:
    void loadRemotes();
    void addDriveCard(const QString &name);
    void startMount(const QString &remote);
    void stopMount(const QString &remote);
    QString findRcloneConfig();

    QListWidget *m_list;
    QLabel *m_statusLabel;
    QMap<QString, DriveState> m_drives;
};