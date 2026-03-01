#pragma once

#include <QOpenGLWidget>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <QVariant>

class MpvWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit MpvWidget(QWidget *parent = nullptr);
    ~MpvWidget();

    void command(const QVariantList &args);
    void setProperty(const QString &name, const QVariant &value);
    QVariant getProperty(const QString &name);

signals:
    void mpvEvents();
    void positionChanged(double timePos);
    void durationChanged(double duration);
    void volumeChanged(double vol);
    void muteChanged(bool muted);
protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private slots:
    void onMpvEvents();

private:
    static void on_mpv_events(void *ctx);
    static void on_mpv_update(void *ctx);

    mpv_handle *m_mpv;
    mpv_render_context *m_mpv_gl;
};