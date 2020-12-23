/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
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
#include "screensaverview.h"
#include "imageprovider.h"

#include <QPixmap>
#include <QProcess>
#include <QQmlEngine>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

Q_GLOBAL_STATIC(QQmlEngine, qmlEngineGlobal)

static QQmlEngine *globalEngine()
{
    if (!qmlEngineGlobal.exists()) {
        qmlEngineGlobal->addImageProvider("deepin-screensaver", new ImageProvider());
        // 添加插件载入路径，方便屏保应用携带插件扩展功能
        qmlEngineGlobal->addImportPath("file://" LIB_PATH "/qml/imports");
        qmlEngineGlobal->addImportPath("qrc:/deepin-screensaver/qml/imports");
        qmlEngineGlobal->addImportPath(QString("file://%1/.local/lib/%2/qml/imports").arg(QDir::homePath(), qApp->applicationName()));
    }

    return qmlEngineGlobal;
}

ScreenSaverView::ScreenSaverView(QWindow *parent)
    : QQuickView(globalEngine(), parent)
{

}

ScreenSaverView::~ScreenSaverView()
{
    stop();
}

bool ScreenSaverView::start(const QString &filePath)
{
    stop();

    if (filePath.endsWith(".qml")) {
        setSource(QUrl(filePath));
    } else {
        if (!m_process) {
            m_process = new QProcess(this);
            m_process->setProcessChannelMode(QProcess::ForwardedChannels);
        }

        create();
        m_process->start(filePath, {"-window-id", QString::number(winId())}, QIODevice::ReadOnly);

        if (!m_process->waitForStarted(3000)) {
            qDebug() << "Failed on start:" << m_process->program() << ", error string:" << m_process->errorString();

            return false;
        }
    }

    return true;
}

void ScreenSaverView::stop()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished();
    }

    // 清理qml的播放
    setSource(QUrl());
    // 清理窗口背景色
    setColor(Qt::black);
}

bool ScreenSaverView::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::MouseButtonDblClick:
    //case QEvent::FocusOut: /* Multiple screen will be triggered, causing the screensaver window to exit */
    case QEvent::ApplicationStateChange:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        emit inputEvent(event->type());
        break;

    default:
        break;
    }

    return QQuickView::event(event);
}
