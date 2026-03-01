#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QPixmap>
#include <QListWidget>
#include <QVBoxLayout>
#include "mpvwidget.h"

class TrackPopup : public QWidget {
    Q_OBJECT
public:
    explicit TrackPopup(QWidget *parent = nullptr);
    void populate(const QList<QPair<int, QString>> &tracks, const QString &type, const QString &currentId);

signals:
    void trackSelected(int index);
    void trackDisabled();

private:
    QListWidget *m_list;
    QPushButton *m_offBtn;
};

class PlayerPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerPage(QWidget *parent = nullptr);
    void play(const QString &filePath, const QString &title, const QPixmap &banner = QPixmap());
    void stop();
    ~PlayerPage();

signals:
    void backRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupLoader();
    void setupOverlay();
    void togglePlayPause();
    void showControls();
    QString formatTime(double seconds);
    void updateVolumeIcon();
    QIcon createWhiteIcon(const QString &path);
    void showCustomMenu(const QString &type);
    void showVolumePopupTemp();
    void saveProgress();
    QList<QPair<int, QString>> getTrackList(const QString &type); 

    MpvWidget *m_player;
    
    QWidget *m_overlay;
    QLabel *m_titleLabel;
    QPushButton *m_playPauseBtn;
    QPushButton *m_volumeBtn;
    QPushButton *m_audioBtn;
    QPushButton *m_subBtn;
    QSlider *m_progressSlider;
    QSlider *m_verticalVolumeSlider;
    QLabel *m_timeLabel;
    QLabel *m_volumeLabel;
    QWidget *m_volumePopup;
    
    QWidget *m_loaderWidget;
    QLabel *m_bannerLabel;          
    QWidget *m_loaderDarkOverlay;   
    QLabel *m_spinnerLabel;         
    QLabel *m_loadingSubText;

    QTimer *m_hideTimer;
    QTimer *m_volHideTimer;

    bool m_isPlaying = false;
    bool m_isMuted = false;
    double m_volume = 100.0;
    double m_duration = 0.0;
    QString m_currentFilePath;
};