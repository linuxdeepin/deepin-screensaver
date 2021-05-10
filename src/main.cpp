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
#include <QGuiApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "screensaver_adaptor.h"
#include "dbusscreensaver.h"

#define APPLICATION_XSTRING(s) APPLICATION_STRING(s)
#define APPLICATION_STRING(s) #s

#ifdef VERSION
    static QString buildVersion(const QString &fallbackVersion)
    {
        QString autoVersion = APPLICATION_XSTRING(VERSION);
        if (autoVersion.isEmpty()) {
            autoVersion = fallbackVersion;
        }
        return autoVersion;
    }
#else
    static QString buildVersion(const QString &fallbackVersion)
    {
        return fallbackVersion;
    }
#endif

int main(int argc, char *argv[])
{
    auto envType = qEnvironmentVariable("XDG_SESSION_TYPE");
    if (envType.contains("wayland")) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }

    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication app(argc, argv);

    app.setOrganizationName("deepin");
    app.setApplicationName("deepin-screensaver");
    app.setApplicationVersion(buildVersion(QMAKE_VERSION));

    bool doStart = true;

    QCommandLineParser parser;

    if (argc > 1) {
        QCommandLineOption option_dbus({"d", "dbus"}, "Register DBus service.");
        QCommandLineOption option_start({"s", "start"}, "Start screen saver.");

        parser.addOption(option_dbus);
        parser.addOption(option_start);
        parser.addPositionalArgument("screensaer-name", "Use the screensaver application.", "[name]");
        parser.addHelpOption();
        parser.addVersionOption();

        parser.process(app);
        doStart = !parser.isSet(option_dbus);
    }

    // 注册DBus服务
    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.\n"
                 "Please check your system settings and try again.\n");

        if (!doStart)
            return -1;
    }

    // add our D-Bus interface and connect to D-Bus
    if (!QDBusConnection::sessionBus().registerService("com.deepin.ScreenSaver")) {
        qWarning("Cannot register the \"com.deepin.ScreenSaver\" service.\n");

        if (!doStart)
            return -1;
    }

    auto server = new DBusScreenSaver();
    Q_UNUSED(new ScreenSaverAdaptor(server))

    if (!QDBusConnection::sessionBus().registerObject("/com/deepin/ScreenSaver", server)) {
        qWarning("Cannot register to the D-Bus object: \"/com/deepin/ScreenSaver\"\n");

        if (!doStart)
            return -1;
    }

    if (doStart) {
        if (argc > 1)
            server->Start(parser.positionalArguments().value(0));
        else
            server->Start();
    }

    return app.exec();
}
