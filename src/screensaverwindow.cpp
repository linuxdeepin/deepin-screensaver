/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "screensaverwindow.h"

#include <QBitmap>
#include <QX11Info>
#include <QAbstractEventDispatcher>
#include <QAbstractNativeEventFilter>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/scrnsaver.h>
#include <X11/extensions/shape.h>

struct xcb_screensaver_notify_event
{
    uint8_t      response_type;
    uint8_t      state; /* ScreenSaverOff, ScreenSaverOn, ScreenSaverCycle*/
    uint8_t      offset;
    uint8_t      kid;
    xcb_window_t window;	    /* screen saver window */
    xcb_window_t root;	    /* root window of event screen */
};

class X11EventFilter : public QAbstractNativeEventFilter
{
public:
    X11EventFilter(ScreenSaverWindow *w, WId wid)
        : window(w)
        , windowId(wid)
    {
        if (!XScreenSaverQueryExtension(QX11Info::display(), &scrnsaver_event_base, &scrnsaver_error_base)) {
            scrnsaver_event_base = scrnsaver_error_base = 0;
        }

        XScreenSaverSelectInput(QX11Info::display(), windowId, ScreenSaverNotifyMask);
    }
    ~X11EventFilter() {

    }

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override
    {
        Q_UNUSED(eventType)
        Q_UNUSED(result)
        xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t*>(message);

        if (scrnsaver_event_base != 0 &&
                event->response_type == scrnsaver_event_base + ScreenSaverNotify) {
            xcb_screensaver_notify_event *se = reinterpret_cast<xcb_screensaver_notify_event*>(event);

            if (se->state == ScreenSaverOn) {
                // ignore
            } else if (se->state == ScreenSaverOff) {
                emit window->screenSaverOff();
            }
        }

        return false;
    }

private:
    ScreenSaverWindow *window;

    WId windowId;
    int scrnsaver_event_base, scrnsaver_error_base;
};

ScreenSaverWindow::ScreenSaverWindow(QWindow *parent)
    : QQuickView(parent)
{
    QPixmap pixmap(1, 1);

    pixmap.fill(Qt::transparent);
    setCursor(pixmap);
}

ScreenSaverWindow::~ScreenSaverWindow()
{
    XScreenSaverUnregister(QX11Info::display(), QX11Info::appScreen());
}

bool ScreenSaverWindow::event(QEvent *event)
{
    if (event->type() == QEvent::PlatformSurface) {
        const QPlatformSurfaceEvent *e = static_cast<QPlatformSurfaceEvent*>(event);

        // 为窗口添加对VisibilityChange事件的监听
        if (e->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
            x11event.reset(new X11EventFilter(this, winId()));

            QAbstractEventDispatcher::instance()->installNativeEventFilter(x11event.data());

            XScreenSaverRegister(QX11Info::display(), QX11Info::appScreen(), winId(), XA_WINDOW);

            // 禁止接收鼠标输入
            XRectangle rects {0, 0, 0, 0};
            XShapeCombineRectangles(QX11Info::display(), winId(), ShapeInput, 0, 0, &rects, 1, 0, 0);
        }
    }

    return QQuickView::event(event);
}
