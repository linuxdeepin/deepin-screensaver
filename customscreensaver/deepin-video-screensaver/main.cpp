// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commandlinemanager.h"
#include "videoscreensaver.h"
#include "xproxywindow.h"

#include <QApplication>
#include <QtDebug>
#include <QWindow>
#include <QMessageBox>
#include <QScreen>
#include <QStringList>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QTimer>
#include <QFileInfo>

static int getDuplicateIndex(const QStringList &args)
{
    int ret = 0;
    if (!args.isEmpty()) {
        QString path = args.first();
        QFileInfo exe(path);
        if (exe.baseName() == "deepin-video-screensaver") {
            auto suffix = exe.suffix();
            bool ok = false;
            int idx = suffix.toInt(&ok);
            if (ok)
                ret = idx;
        }
    }
    return ret;
}

int main(int argc, char *argv[])
{
    auto envType = qEnvironmentVariable("XDG_SESSION_TYPE");
    if (envType.contains("wayland")) {
        qWarning() << "change wayland to xcb for QT_QPA_PLATFORM.";
        qputenv("QT_QPA_PLATFORM", "xcb");
        qputenv("XDG_SESSION_TYPE", "x11");
        qunsetenv("WAYLAND_DISPLAY");
    }

    QApplication a(argc, argv);
    a.setOrganizationName("deepin");
    a.setApplicationName("deepin-video-screensaver");
#ifdef ENABLE_DMR
    // required by mpv
    setlocale(LC_NUMERIC, "C");
#endif

    int index = getDuplicateIndex(a.arguments());
    qInfo() << "Duplicate Index" << index;

    CommandLineManager::instance()->process(a.arguments());

    if (CommandLineManager::instance()->isSet("window-id")) {

        QString windowId = CommandLineManager::instance()->value("window-id");
        WId windowHwnd = WId(windowId.toULongLong());

        XProxyWindow view(windowHwnd);
        if (!view.init())
            return -1;

        VideoScreensaver screensaver;
        screensaver.init(&view, index);

        view.show();
        return a.exec();
    } else {

        VideoScreensaver screensaver;
        screensaver.init(nullptr, index);
        auto view = screensaver.view();

        view->resize(800, 600);
        view->show();
        return a.exec();
    }

    return 0;
}
