// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xproxywindow.h"

#include <QWindow>
#include <QX11Info>
#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>

#include <xcb/xcb.h>
#include <X11/Xlib.h>

XProxyWindow::XProxyWindow(WId parent)
    : QWidget(nullptr), m_parentId(parent)
{
    Q_ASSERT(parent);
    setAttribute(Qt::WA_DeleteOnClose);
}

bool XProxyWindow::init()
{
    QWindow *window = QWindow::fromWinId(m_parentId);
    if (!window) {
        qCritical() << "invalid window id" << m_parentId;
        return false;
    }

    auto lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    setWindowFlag(Qt::WindowTransparentForInput, true);

    setProperty("_q_embedded_native_parent_handle", QVariant(m_parentId));
    winId();
    windowHandle()->setParent(window);

    XSelectInput(QX11Info::display(), m_parentId, StructureNotifyMask);
    qApp->installNativeEventFilter(this);

    XWindowAttributes attr;
    XGetWindowAttributes(QX11Info::display(), Window(m_parentId), &attr);

    QSize widSize = mapFromHandle(QSize(attr.width, attr.height));
    qInfo() << "set window size" << widSize;
    setGeometry(QRect(QPoint(0, 0), widSize));

    return true;
}

bool XProxyWindow::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
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
            if (ce->window == Window(m_parentId)) {
                qInfo() << "parent window closed";
                QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
            }
        }
    }
    return false;
}

void XProxyWindow::closeEvent(QCloseEvent *event)
{
    for (auto wid : findChildren<QWidget *>())
        wid->close();

    QWidget::closeEvent(event);
}

QSize XProxyWindow::mapFromHandle(const QSize &handleSize)
{
    qreal ratio = devicePixelRatioF();
    qDebug() << "parent window handle size" << handleSize << "devicePixelRatio" << ratio;

    if (ratio > 0 && ratio != 1.0)
        return handleSize / ratio;
    else
        return handleSize;
}
