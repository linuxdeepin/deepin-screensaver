// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "slideshowscreensaver.h"
#include "slideshowconfig.h"

#include <DApplication>

#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QWindow>
#include <QScreen>
#include <QPainter>
#include <QPaintEvent>
#include <QRandomGenerator>
#include <QImageReader>

#include <xcb/xcb.h>
#include <X11/Xlib.h>

#define IMAGE_MAX_SIZE 30 * 1024 * 1024   // Only display image smaller than 30M

DWIDGET_USE_NAMESPACE

SlideshowScreenSaver::SlideshowScreenSaver(bool subWindow, QWidget *parent)
    : QWebEngineView(parent), m_subWindow(subWindow)
{
    if (m_subWindow)
        setWindowFlag(Qt::WindowTransparentForInput, true);
}

SlideshowScreenSaver::~SlideshowScreenSaver()
{
}

void SlideshowScreenSaver::closeEvent(QCloseEvent *event)
{
    for (auto wid : findChildren<QWidget *>())
        wid->close();

    QWidget::closeEvent(event);
}

bool SlideshowScreenSaver::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(result);
    if (eventType == "xcb_generic_event_t") {
        xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t *>(message);
        int type = (event->response_type & ~0x80);
        if (XCB_CONFIGURE_NOTIFY == type) {
            xcb_configure_notify_event_t *ce = reinterpret_cast<xcb_configure_notify_event_t *>(event);
            qInfo() << "parent window size changed" << ce->width << ce->height;
            QSize widSize = mapFromHandle(QSize(ce->width, ce->height));
            if (widSize != size()) {
                qInfo() << "update window size:" << widSize;
                resize(widSize);
            }
        } else if (XCB_DESTROY_NOTIFY == type) {
            xcb_destroy_notify_event_t *ce = reinterpret_cast<xcb_destroy_notify_event_t *>(event);
            if (ce->window == Window(this->windowHandle()->winId())) {
                qInfo() << "parent window closed";
                QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
            }
        }
    }
    return false;
}

void SlideshowScreenSaver::init()
{
    connect(this, &QWebEngineView::loadFinished, [&](bool success) {
        if (!success) {
            if (m_url.startsWith(QStringLiteral("https://")))
                m_url.replace(QStringLiteral("https://"), QStringLiteral("http://"));
                SlideshowConfig::instance()->m_dCfg->setValue(kSlideshowPath, m_url);
            this->load(QUrl(m_url));
        }
    });
    updateViewSize(this->size());
    loadSlideshowImages();
}

void SlideshowScreenSaver::loadSlideshowImages()
{
    m_url = SlideshowConfig::instance()->slideshowPath();
    this->load(QUrl(m_url));
}

QSize SlideshowScreenSaver::mapFromHandle(const QSize &handleSize)
{
    qreal ratio = devicePixelRatioF();
    qDebug() << "parent window handle size" << handleSize << "devicePixelRatio" << ratio;

    if (ratio > 0 && ratio != 1.0)
        return handleSize / ratio;
    else
        return handleSize;
}

void SlideshowScreenSaver::updateViewSize(const QSize &size)
{
    QSize updateSize = mapFromHandle(size);

    if (updateSize != size){
        this->resize(updateSize);
    }
}
