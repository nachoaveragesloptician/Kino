#include "mpvwidget.h"
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <stdint.h>
#include <QSettings>
MpvWidget::MpvWidget(QWidget *parent) : QOpenGLWidget(parent), m_mpv_gl(nullptr)
{
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
    m_mpv = mpv_create();
    if (!m_mpv) qFatal("Could not create mpv context");

    QSettings settings("Kino", "AppConfig");
    bool useGpu = settings.value("gpu_accel", true).toBool();
    int cacheMb = settings.value("mpv_cache_mb", 150).toInt();

    int64_t wid = static_cast<int64_t>(this->winId());
    mpv_set_option(m_mpv, "wid", MPV_FORMAT_INT64, &wid);
    mpv_set_option_string(m_mpv, "osc", "no");
    mpv_set_option_string(m_mpv, "input-default-bindings", "yes");
    mpv_set_option_string(m_mpv, "vo", "libmpv"); 

    if (useGpu) {
        mpv_set_option_string(m_mpv, "hwdec", "auto-safe"); 
    } else {
        mpv_set_option_string(m_mpv, "hwdec", "no");
    }

    mpv_set_option_string(m_mpv, "cache", "yes");
    mpv_set_option_string(m_mpv, "demuxer-max-bytes", QString::number(cacheMb * 1024 * 1024).toUtf8().constData());

    QString watchLaterDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/Kino/watch_later";
    QDir().mkpath(watchLaterDir);
    mpv_set_option_string(m_mpv, "watch-later-directory", watchLaterDir.toUtf8().constData());

    mpv_initialize(m_mpv);
    mpv_request_log_messages(m_mpv, "info");

    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "mute", MPV_FORMAT_FLAG);

    mpv_set_wakeup_callback(m_mpv, on_mpv_events, this);
    connect(this, &MpvWidget::mpvEvents, this, &MpvWidget::onMpvEvents, Qt::QueuedConnection);
}

MpvWidget::~MpvWidget()
{
    makeCurrent();
    if (m_mpv_gl) mpv_render_context_free(m_mpv_gl);
    mpv_terminate_destroy(m_mpv);
    doneCurrent();
}

void MpvWidget::command(const QVariantList &args)
{
    QByteArray arr[20];
    const char *cmd[21];
    int n = qMin(args.count(), 20);
    for (int i = 0; i < n; i++) {
        arr[i] = args[i].toString().toUtf8();
        cmd[i] = arr[i].constData();
    }
    cmd[n] = nullptr;
    
    mpv_command_async(m_mpv, 0, cmd); 
}

void MpvWidget::setProperty(const QString &name, const QVariant &value) {
    QByteArray n = name.toUtf8();
    QByteArray v = value.toString().toUtf8();
    mpv_set_property_string(m_mpv, n.constData(), v.constData());
}

QVariant MpvWidget::getProperty(const QString &name) {
    char *str = nullptr;
    mpv_get_property(m_mpv, name.toUtf8().constData(), MPV_FORMAT_STRING, &str);
    if (!str) return QVariant();
    QString res = QString::fromUtf8(str);
    mpv_free(str);
    return res;
}

void MpvWidget::initializeGL()
{
    mpv_opengl_init_params gl_init_params{
        [](void *, const char *name) -> void* {
            QOpenGLContext *ctx = QOpenGLContext::currentContext();
            return ctx ? reinterpret_cast<void*>(ctx->getProcAddress(QByteArray(name))) : nullptr;
        },
        nullptr
    };
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    if (mpv_render_context_create(&m_mpv_gl, m_mpv, params) < 0) qFatal("Failed to init mpv GL");
    mpv_render_context_set_update_callback(m_mpv_gl, on_mpv_update, this);
}

void MpvWidget::paintGL()
{
    int w = width() * devicePixelRatio();
    int h = height() * devicePixelRatio();
    mpv_opengl_fbo mpfbo{static_cast<int>(defaultFramebufferObject()), w, h, 0};
    int flip_y = 1;

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    mpv_render_context_render(m_mpv_gl, params);
}

void MpvWidget::resizeGL(int, int) {}

void MpvWidget::onMpvEvents()
{
    while (m_mpv) {
        mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) break;

        if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            mpv_event_property *prop = (mpv_event_property*)event->data;
            if (QString(prop->name) == "time-pos" && prop->format == MPV_FORMAT_DOUBLE) {
                emit positionChanged(*(double*)prop->data);
            } else if (QString(prop->name) == "duration" && prop->format == MPV_FORMAT_DOUBLE) {
                emit durationChanged(*(double*)prop->data);
            } else if (QString(prop->name) == "volume" && prop->format == MPV_FORMAT_DOUBLE) {
                emit volumeChanged(*(double*)prop->data);
            } else if (QString(prop->name) == "mute" && prop->format == MPV_FORMAT_FLAG) {
                emit muteChanged(*(int*)prop->data != 0);
            }
        } 
        else if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
            struct mpv_event_log_message *msg = (struct mpv_event_log_message *)event->data;
            QString logText = QString::fromUtf8(msg->text).trimmed();
            
            if (logText.contains("Cannot load libcuda.so.1")) {
                continue; 
            }

            qDebug().noquote() << QString("[MPV %1|%2] %3")
                                    .arg(msg->level)
                                    .arg(msg->prefix)
                                    .arg(logText);
        }
    }
}

void MpvWidget::on_mpv_events(void *ctx) { QMetaObject::invokeMethod((MpvWidget*)ctx, "mpvEvents", Qt::QueuedConnection); }
void MpvWidget::on_mpv_update(void *ctx) { QMetaObject::invokeMethod((MpvWidget*)ctx, "update", Qt::QueuedConnection); }