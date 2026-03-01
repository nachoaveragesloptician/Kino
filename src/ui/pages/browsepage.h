#pragma once

#include <QWidget>
#include <QListView>
#include <QFileSystemModel>
#include <QLabel>
#include <QPushButton>
#include <QStyledItemDelegate>

class BrowseDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit BrowseDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class BrowsePage : public QWidget
{
    Q_OBJECT
public:
    explicit BrowsePage(QWidget *parent = nullptr);
    void reload();
    void refresh();

signals:
    void playVideoRequested(const QString &filePath, const QString &title, const QPixmap &banner);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onDirectoryLoaded(const QString &path);
    void onItemDoubleClicked(const QModelIndex &index);
    void onPlayButtonClicked(const QString &path, const QString &title);

private:
    QListView *m_view;
    QFileSystemModel *m_model;
    QPushButton *m_backBtn;
    QLabel *m_pathLabel;
    
    QWidget *m_emptyWidget;
    
    QString m_rootPath;
    QString m_currentPath;
    
    void checkEmptyState();
    bool isMediaFile(const QString &fileName) const;
};