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
#include "dbusscreensaver.h"
#include "screensaverwindow.h"

#include <QDebug>
#include <QGuiApplication>
#include <QSettings>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QResource>
#include <QStandardPaths>
#include <QDirIterator>
#include <QTimer>
#include <QX11Info>
#include <QAbstractEventDispatcher>
#include <QAbstractNativeEventFilter>

#include <xcb/xcb.h>
#include <X11/Xproto.h>
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
    X11EventFilter(DBusScreenSaver *w)
        : screenSaver(w)
    {
        if (!XScreenSaverQueryExtension(QX11Info::display(), &scrnsaver_event_base, &scrnsaver_error_base)) {
            scrnsaver_event_base = scrnsaver_error_base = 0;
        }

        // 不要显示
        puppetWindow.create();

        XScreenSaverSelectInput(QX11Info::display(), puppetWindow.winId(), ScreenSaverNotifyMask);
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
                emit screenSaver->stop();
            }
        }

        return false;
    }

private:
    DBusScreenSaver *screenSaver;

    QWindow puppetWindow;
    int scrnsaver_event_base, scrnsaver_error_base;
};

DBusScreenSaver::DBusScreenSaver(QObject *parent)
    : QObject(parent)
    , m_resourceDirList({QDir(RESOURCE_PATH)})
    , m_moduleDirList({QDir("://deepin-screensaver/modules"), QDir(MODULE_PATH)})
    , m_settings(qApp->organizationName(), qApp->applicationName())
{
    m_moduleDirList.prepend(QDir(QString("%1/.local/lib/%2/modules").arg(QDir::homePath(), qApp->applicationName())));
    m_resourceDirList.prepend(QDir(QString("%1/.local/lib/%2/resources").arg(QDir::homePath(), qApp->applicationName())));

    m_autoQuitTimer.setInterval(30000);
    m_autoQuitTimer.setSingleShot(true);
#ifndef QT_DEBUG
    m_autoQuitTimer.start();
#endif
    m_currentScreenSaver = m_settings.value("currentScreenSaver").toString();
    m_lockScreenAtAwake = m_settings.value("lockScreenAtAwake", false).toBool();
    m_lockScreenDelay = m_settings.value("lockScreenDelay", 15).toInt();
    m_lockScreenTimer.setInterval(m_lockScreenDelay * 1000);
    m_lockScreenTimer.setSingleShot(true);

    connect(&m_autoQuitTimer, &QTimer::timeout, this, &QCoreApplication::quit);

    QDBusConnection::sessionBus().connect("com.deepin.daemon.Power",
                                          "/com/deepin/daemon/Power",
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          this, SLOT(onDBusPropertyChanged(QString,QVariantMap,QDBusMessage)));

    RefreshScreenSaverList();
}

DBusScreenSaver::~DBusScreenSaver()
{
    if (isRunning()) {
        Stop();
    }

    clearResourceList();
}

bool DBusScreenSaver::Preview(const QString &name, int staysOn, bool preview)
{
    const QDir &moduleDir = m_screenSaverNameToDir.value(name);

    if (!QFile::exists(moduleDir.absoluteFilePath(name)))
        return false;

    if (preview) {
        if (x11event) {
            QAbstractEventDispatcher::instance()->removeNativeEventFilter(x11event.data());
            x11event.reset(nullptr);
        }

        qGuiApp->restoreOverrideCursor();
    } else {
        // 安装native事件监控，收到XScreenSaverOff事件时退出屏保
        if (!x11event) {
            x11event.reset(new X11EventFilter(this));
            QAbstractEventDispatcher::instance()->installNativeEventFilter(x11event.data());
        }

        qGuiApp->setOverrideCursor(Qt::BlankCursor);
    }

    ensureWindowMap();

    for (ScreenSaverWindow *window : m_windowMap) {
        if (name.endsWith(".qml")) {
            if (moduleDir.path().startsWith(":/"))
                window->start("qrc" + moduleDir.absoluteFilePath(name));
            else
                window->start("file://" + moduleDir.absoluteFilePath(name));
        } else {
            window->start(moduleDir.absoluteFilePath(name));
        }

        if (staysOn) {
            window->setFlags(window->flags() | Qt::WindowStaysOnTopHint);
        } else {
            window->setFlags(window->flags() | Qt::WindowStaysOnBottomHint);
        }

        if (!preview) {
            connect(window, &ScreenSaverWindow::inputEvent, this, &DBusScreenSaver::stop, Qt::UniqueConnection);
        } else {
            disconnect(window, &ScreenSaverWindow::inputEvent, this, &DBusScreenSaver::stop);
        }

        window->showFullScreen();

        QTimer::singleShot(500, window, [window] {
            // 在kwin中，窗口类型为Qt::Drawer时会导致多屏情况下只会有一个窗口被显示，另一个被最小化
            // 这里判断最小化的窗口后更改其窗口类型再次显示。

            bool is_hidden = window->visibility() == QWindow::Minimized;

            if (!is_hidden) {
                // KWin 上开启 HiddenPreviews=6 配置后，无法直接判断出窗口的状态，因此fallback到读取窗口属性判断其是否被隐藏
                Atom atom;
                int format;
                ulong nitems;
                ulong bytes_after_return;
                uchar *prop_datas;
                XGetWindowProperty(QX11Info::display(),
                                   window->winId(),
                                   XInternAtom(QX11Info::display(), "_NET_WM_STATE", true),
                                   0, 1024, false,
                                   XA_ATOM, &atom,
                                   &format, &nitems,
                                   &bytes_after_return, &prop_datas);

                if (prop_datas && format == 32 && atom == XA_ATOM) {
                    const Atom *states = reinterpret_cast<const Atom *>(prop_datas);
                    const Atom *statesEnd = states + nitems;
                    if (statesEnd != std::find(states, statesEnd, XInternAtom(QX11Info::display(), "_NET_WM_STATE_HIDDEN", true)))
                        is_hidden = true;
                    XFree(prop_datas);
                }
            }

            if (is_hidden) {
                window->setFlags(window->flags() & ~Qt::Dialog | Qt::Window);
                window->close();
                window->showFullScreen();
            }
        });
    }

    m_autoQuitTimer.stop();

    emit isRunningChanged(true);

    return true;
}

static QString getAppRuntimePath() {
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation));

    dir.mkdir(qApp->applicationName());

    return dir.absoluteFilePath(qApp->applicationName());
}

QString DBusScreenSaver::GetScreenSaverCover(const QString &name) const
{
    if (name.isEmpty())
        return QString();

    QDir moduleDir = m_screenSaverNameToDir.value(name);

    // 资源文件中的封面图被复制到了其它路径
    if (moduleDir.path().startsWith(":/")) {
        moduleDir.setPath(getAppRuntimePath());
    }

    QString cover = moduleDir.absoluteFilePath(QString("cover/%1.svg").arg(name));

    if (QFile::exists(cover))
        return cover;

    cover = moduleDir.absoluteFilePath(QString("cover/%1.bmp").arg(name));

        if (QFile::exists(cover))
            return cover;

    cover = moduleDir.absoluteFilePath(QString("cover/%1.jpg").arg(name));

    if (QFile::exists(cover))
        return cover;

    cover = moduleDir.absoluteFilePath(QString("cover/%1.png").arg(name));

    if (QFile::exists(cover))
        return cover;

    return QString();
}

// 连目录本身都复制
static bool copyDir(QDir source, QDir target)
{
    target.mkdir(source.dirName());

    if (!target.cd(source.dirName())) {
        return false;
    }

    source.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QDirIterator di(source);

    while (di.hasNext()) {
        di.next();

        const QString &target_file = target.absoluteFilePath(di.fileName());

        if (!QFile::exists(target_file))
            QFile::copy(di.filePath(), target_file);
    }

    return true;
}

void DBusScreenSaver::RefreshScreenSaverList()
{
    // 重新加载资源文件
    clearResourceList();

    for (const QDir &resourceDir : m_resourceDirList) {
        for (const QFileInfo &info : resourceDir.entryInfoList({"*.rcc"}, QDir::Files)) {
            if (QResource::registerResource(info.absoluteFilePath())) {
                m_resourceList << info.absoluteFilePath();
            } else {
                qWarning() << "Failed on load resource file:" << info.absoluteFilePath();
            }
        }
    }

    m_screenSaverList.clear();
    m_screenSaverNameToDir.clear();

    for (const QDir &moduleDir : m_moduleDirList) {
        for (const QFileInfo &info : moduleDir.entryInfoList(QDir::Files)) {
            if (!info.isExecutable() && info.suffix() != "qml")
                continue;

            m_screenSaverList.append(info.fileName());
            m_screenSaverNameToDir[info.fileName()] = moduleDir;
        }

        // 特殊处理资源文件, 将资源文件中的封面图复制到一个临时目录
        if (moduleDir.path().startsWith(":/")) {
            QDir cover_dir(moduleDir.filePath("cover"));

            if (!copyDir(cover_dir, getAppRuntimePath())) {
                qWarning() << "Failed Copy the" << cover_dir << "directory to" << getAppRuntimePath();
            }
        }
    }

    if (!m_screenSaverList.isEmpty()) {
        if (!m_screenSaverList.contains(m_currentScreenSaver))
            m_currentScreenSaver = m_screenSaverList.first();
    }

    emit allScreenSaverChanged(m_screenSaverList);
}

void DBusScreenSaver::Start(const QString &name)
{
    if (isRunning())
        return;

    Preview(name.isEmpty() ? m_currentScreenSaver : name, 1, false);

    // 计时用于判断在唤醒时是否要锁定屏幕
    m_lockScreenTimer.start();
}

void DBusScreenSaver::Stop(bool lock)
{
    if (m_windowMap.isEmpty())
        return;

    // 只在由窗口自己唤醒时才会触发锁屏
    if (m_lockScreenAtAwake && !m_lockScreenTimer.isActive()
            && (lock || qobject_cast<ScreenSaverWindow*>(sender()))) {
        QDBusInterface lockDBus("com.deepin.dde.lockFront", "/com/deepin/dde/lockFront",
                                "com.deepin.dde.lockFront");

        // 通过DBus拉起锁屏程序
        lockDBus.call("Show");
    }

    if (x11event) {
        QAbstractEventDispatcher::instance()->removeNativeEventFilter(x11event.data());
        x11event.reset(nullptr);
    }

    for (ScreenSaverWindow *w : m_windowMap) {
        cleanWindow(w);
    }

    m_windowMap.clear();

#ifndef QT_DEBUG
    m_autoQuitTimer.start();
#endif

    emit isRunningChanged(false);
}

void DBusScreenSaver::stop()
{
    Stop(true);
}

QStringList DBusScreenSaver::allScreenSaver() const
{
    return m_screenSaverList;
}

int DBusScreenSaver::batteryScreenSaverTimeout() const
{
    QDBusInterface remoteApp( "com.deepin.daemon.Power", "/com/deepin/daemon/Power",
                              "com.deepin.daemon.Power" );

    return remoteApp.property("BatteryScreensaverDelay").toInt();
}

int DBusScreenSaver::linePowerScreenSaverTimeout() const
{
    QDBusInterface remoteApp( "com.deepin.daemon.Power", "/com/deepin/daemon/Power",
                              "com.deepin.daemon.Power" );

    return remoteApp.property("LinePowerScreensaverDelay").toInt();
}

QString DBusScreenSaver::currentScreenSaver() const
{
    return m_currentScreenSaver;
}

bool DBusScreenSaver::isRunning() const
{
    return !m_windowMap.isEmpty();
}

void DBusScreenSaver::setBatteryScreenSaverTimeout(int batteryScreenSaverTimeout)
{
    QDBusInterface remoteApp( "com.deepin.daemon.Power", "/com/deepin/daemon/Power",
                              "com.deepin.daemon.Power" );

    remoteApp.setProperty("BatteryScreensaverDelay", batteryScreenSaverTimeout);
}

void DBusScreenSaver::setLinePowerScreenSaverTimeout(int linePowerScreenSaverTimeout)
{
    QDBusInterface remoteApp( "com.deepin.daemon.Power", "/com/deepin/daemon/Power",
                              "com.deepin.daemon.Power" );

    remoteApp.setProperty("LinePowerScreensaverDelay", linePowerScreenSaverTimeout);
}

void DBusScreenSaver::setCurrentScreenSaver(QString currentScreenSaver)
{
    if (m_currentScreenSaver == currentScreenSaver)
        return;

    m_currentScreenSaver = currentScreenSaver;
    m_settings.setValue("currentScreenSaver", m_currentScreenSaver);

    emit currentScreenSaverChanged(m_currentScreenSaver);
}

bool DBusScreenSaver::lockScreenAtAwake() const
{
    return m_lockScreenAtAwake;
}

int DBusScreenSaver::lockScreenDelay() const
{
    return m_lockScreenDelay;
}

void DBusScreenSaver::setLockScreenAtAwake(bool lockScreenAtAwake)
{
    if (m_lockScreenAtAwake == lockScreenAtAwake)
        return;

    m_lockScreenAtAwake = lockScreenAtAwake;
    m_settings.setValue("lockScreenAtAwake", m_lockScreenAtAwake);

    emit lockScreenAtAwakeChanged(m_lockScreenAtAwake);
}

void DBusScreenSaver::setLockScreenDelay(int lockScreenDelay)
{
    if (m_lockScreenDelay == lockScreenDelay)
        return;

    m_lockScreenDelay = lockScreenDelay;
    m_settings.setValue("lockScreenDelay", m_lockScreenDelay);
    m_lockScreenTimer.setInterval(m_lockScreenDelay * 1000);

    emit lockScreenDelayChanged(m_lockScreenDelay);
}

void DBusScreenSaver::onDBusPropertyChanged(const QString &interface, const QVariantMap &changed_properties, const QDBusMessage &message)
{
    Q_UNUSED(interface)
    Q_UNUSED(message)

    auto begin = changed_properties.constBegin();

    while (begin != changed_properties.constEnd()) {
        if (QLatin1Literal("BatteryScreensaverDelay") == begin.key()) {
            bool ok = false;
            int value = begin.value().toInt(&ok);

            if (ok)
                emit batteryScreenSaverTimeoutChanged(value);
            else
                qWarning() << "Failed the value cast to int type, name:" << begin.key() << "value:" << begin.value();
        } else if (QLatin1Literal("LinePowerScreensaverDelay") == begin.key()) {
            bool ok = false;
            int value = begin.value().toInt(&ok);

            if (ok)
                emit linePowerScreenSaverTimeoutChanged(value);
            else
                qWarning() << "Failed the value cast to int type, name:" << begin.key() << "value:" << begin.value();
        }

        ++begin;
    }
}

void DBusScreenSaver::clearResourceList()
{
    for (const QString &name : m_resourceList) {
        QResource::unregisterResource(name);
    }

    m_resourceList.clear();
}

void DBusScreenSaver::ensureWindowMap()
{
    if (!m_windowMap.isEmpty())
        return;

    connect(qGuiApp, &QGuiApplication::screenAdded, this, &DBusScreenSaver::onScreenAdded, Qt::UniqueConnection);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &DBusScreenSaver::onScreenRemoved, Qt::UniqueConnection);

    for (QScreen *s : qGuiApp->screens()) {
        onScreenAdded(s);
    }
}

void DBusScreenSaver::onScreenAdded(QScreen *s)
{
    // 列表为空时说明未初始化，此时不用响应屏幕add信号
    if (sender() && m_windowMap.isEmpty())
        return;

    if (m_windowMap.contains(s))
        return;

    ScreenSaverWindow *w = new ScreenSaverWindow();

    // 必须把窗口移动到屏幕所在位置，否则窗口被创建时会被移动到0,0所在的屏幕
    w->setPosition(s->availableGeometry().center());
    w->setScreen(s);
    // kwin 下不可添加Dialog标志，否则会导致窗口只能显示出一个，然而，在 deepin-wm 上使用其它窗口类型时又会有动画
    // 不要添加Qt::WindowDoesNotAcceptFocus，以防止deepin-kwin在进入到显示桌面模式时触发屏幕保护
    // 但是却没有退出显示桌面模式，这样将会导致dock显示在屏幕保护窗口的上方
    w->setFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::Drawer);
    w->create();

    m_windowMap[s] = w;

#ifdef QT_DEBUG
    // 禁止接收鼠标输入
//    XRectangle rects {0, 0, 0, 0};
//    XShapeCombineRectangles(QX11Info::display(), w->winId(), ShapeInput, 0, 0, &rects, 1, 0, 0);
#endif

    QFunctionPointer screenNumber = qGuiApp->platformFunction("XcbVirtualDesktopNumber");

    if (screenNumber) {
        int number = (*reinterpret_cast<int(*)(QScreen *s)>(screenNumber))(s);

        s->setProperty("_d_x11_screen_number", number);
        XScreenSaverRegister(QX11Info::display(), number, w->winId(), XA_WINDOW);
    }
}

void DBusScreenSaver::onScreenRemoved(QScreen *s)
{
    if (ScreenSaverWindow *w = m_windowMap.take(s)) {
        cleanWindow(w);
    }
}

void DBusScreenSaver::cleanWindow(ScreenSaverWindow *w)
{
    w->hide();
    w->stop();
    w->deleteLater();

    if (QScreen *s = w->screen()) {
        bool ok = false;
        int screenNumber = s->property("_d_x11_screen_number").toInt(&ok);

        if (ok) {
            XScreenSaverUnregister(QX11Info::display(), screenNumber);
        }
    }
}
