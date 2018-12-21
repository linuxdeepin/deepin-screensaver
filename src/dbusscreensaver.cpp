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
#include "imageprovider.h"

#include <QDebug>
#include <QCoreApplication>
#include <QSettings>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QResource>
#include <QStandardPaths>
#include <QDirIterator>

DBusScreenSaver::DBusScreenSaver(QObject *parent)
    : QObject(parent)
    , m_resourceDirList({QDir(RESOURCE_PATH)})
    , m_moduleDirList({QDir("://deepin-screensaver/modules"), QDir(MODULE_PATH)})
    , m_settings(qApp->organizationName())
{
    m_autoQuitTimer.setInterval(30000);
    m_autoQuitTimer.setSingleShot(true);
    m_autoQuitTimer.start();
    m_currentScreenSaver = m_settings.value("currentScreenSaver").toString();

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
    m_window->deleteLater();

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

    if (!m_window) {
        m_window = new ScreenSaverWindow();
        m_window->setFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::Drawer | Qt::WindowDoesNotAcceptFocus);
        m_window->create();

        static ImageProvider *ip = new ImageProvider();

        m_window->engine()->addImageProvider("deepin-screensaver", ip);
    }

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished();
    }

    // 清理qml的播放
    m_window->setSource(QUrl());
    // 清理窗口背景色
    m_window->setColor(Qt::black);

    if (name.endsWith(".qml")) {
        if (moduleDir.path().startsWith(":/"))
            m_window->setSource(QUrl("qrc" + moduleDir.absoluteFilePath(name)));
        else
            m_window->setSource(QUrl::fromLocalFile(moduleDir.absoluteFilePath(name)));
    } else {
        if (!m_process) {
            m_process = new QProcess(this);
            m_process->setProcessChannelMode(QProcess::ForwardedChannels);
        }

        m_process->start(moduleDir.absoluteFilePath(name), {"-window-id", QString::number(m_window->winId())}, QIODevice::ReadOnly);

        if (!m_process->waitForStarted(3000)) {
            qDebug() << "Failed on start:" << m_process->program() << ", error string:" << m_process->errorString();

            return false;
        }
    }

    if (staysOn) {
        m_window->setFlag(Qt::WindowStaysOnTopHint);
    } else {
        m_window->setFlag(Qt::WindowStaysOnBottomHint);
    }

    if (!preview) {
        connect(m_window, &ScreenSaverWindow::screenSaverOff, this, &DBusScreenSaver::Stop, Qt::UniqueConnection);
    } else {
        disconnect(m_window, &ScreenSaverWindow::screenSaverOff, this, &DBusScreenSaver::Stop);
    }

    m_window->showFullScreen();
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
}

void DBusScreenSaver::Stop()
{
    m_window->removeEventFilter(this);
    m_window->hide();
    m_autoQuitTimer.start();

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished();
    }

    m_window->setSource(QUrl());

    emit isRunningChanged(false);
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
    return (m_process && m_process->state() != QProcess::NotRunning)
            || (m_window && m_window->isVisible() && m_window->source().isValid());
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
