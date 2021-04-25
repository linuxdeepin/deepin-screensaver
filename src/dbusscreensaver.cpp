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
#include <QWindow>
#include <QThread>

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
    m_lockScreenDelay = m_settings.value("lockScreenDelay", 1).toInt();  //设置屏保静置时间,超过该时间唤醒后锁屏
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
        if (staysOn) {
            window->setFlags(window->flags() | Qt::WindowStaysOnTopHint, !preview);
        } else {
            window->setFlags(window->flags() | Qt::WindowStaysOnBottomHint, false);
        }

        if (name.endsWith(".qml")) {
            if (moduleDir.path().startsWith(":/"))
                window->start("qrc" + moduleDir.absoluteFilePath(name));
            else
                window->start("file://" + moduleDir.absoluteFilePath(name));
        } else {
            window->start(moduleDir.absoluteFilePath(name));
        }

        if (!preview) {
            connect(window, &ScreenSaverWindow::inputEvent, this, &DBusScreenSaver::stop, Qt::UniqueConnection);
        } else {
            disconnect(window, &ScreenSaverWindow::inputEvent, this, &DBusScreenSaver::stop);
        }

        window->show();
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

    bool waitLock = false;
    // 只在由窗口自己唤醒时才会触发锁屏
    if (m_lockScreenAtAwake && !m_lockScreenTimer.isActive()
            && (lock || qobject_cast<ScreenSaverWindow*>(sender()))) {
        QDBusInterface lockDBus("com.deepin.dde.lockFront", "/com/deepin/dde/lockFront",
                                "com.deepin.dde.lockFront");

        // 通过DBus拉起锁屏程序
        lockDBus.call("Show");
        waitLock = true;
    }

    if (x11event) {
        QAbstractEventDispatcher::instance()->removeNativeEventFilter(x11event.data());
        x11event.reset(nullptr);
    }

    //等待锁屏界面显示，防止闪现桌面
    if (waitLock)
        QThread::msleep(400);

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
    static const QString saverpic("saverpic.qml");
    if (m_screenSaverList.contains(saverpic)) {
        // 根据要求，对外提供的屏保程序列表，不包含自定义图片屏保程序
        QStringList tmpList = m_screenSaverList;
        tmpList.removeOne(saverpic);
        return tmpList;
    }
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
    w->setScreen(s);

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
