#pragma once

#include <QWidget>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

class SidebarItem : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float hoverProgress READ hoverProgress WRITE setHoverProgress)
    Q_PROPERTY(float activeProgress READ activeProgress WRITE setActiveProgress)

public:
    explicit SidebarItem(const QIcon &icon, const QString &text, const QString &path, QWidget *parent = nullptr);

    void setActive(bool active);
    QString path() const { return m_path; }

    float hoverProgress() const { return m_hoverProgress; }
    void setHoverProgress(float p) { m_hoverProgress = p; update(); }

    float activeProgress() const { return m_activeProgress; }
    void setActiveProgress(float p) { m_activeProgress = p; update(); }

signals:
    void clicked(const QString &path);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return QSize(80, 70); }

private:
    QString m_path;
    QString m_text;
    QIcon m_icon;
    bool m_isActive = false;

    float m_hoverProgress = 0.0f;
    float m_activeProgress = 0.0f;

    QPropertyAnimation *m_hoverAnim;
    QPropertyAnimation *m_activeAnim;
};

class Sidebar : public QWidget {
    Q_OBJECT

public:
    explicit Sidebar(QWidget *parent = nullptr);

signals:
    void directorySelected(const QString &path);

private:
    void addItem(const QIcon &icon, const QString &text, const QString &path);
    void addLogo();
    QList<SidebarItem*> m_items;
};