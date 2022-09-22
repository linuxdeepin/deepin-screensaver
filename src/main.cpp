// SPDX-FileCopyrightText: 2017 ~ 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <DApplication>

#include <QCommandLineParser>
#include <QCommandLineOption>

#include "screensaver_adaptor.h"
#include "dbusscreensaver.h"
#include "customconfig.h"

DWIDGET_USE_NAMESPACE

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
        qInfo() << QDateTime::currentDateTime().toString() << "notes:change wayland to xcb for QT_QPA_PLATFORM.";
        qputenv("QT_QPA_PLATFORM", "xcb");
    }

    DApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    DApplication app(argc, argv);

    app.setOrganizationName("deepin");
    app.setApplicationName("deepin-screensaver");
    app.setApplicationVersion(buildVersion(QMAKE_VERSION));

    bool doStart = true;
    bool isCustomConfig = false;
    QString configName;

    QCommandLineParser parser;
    if (argc > 1) {
        QCommandLineOption optionDbus({"d", "dbus"}, "Register DBus service.");
        QCommandLineOption optionStart({"s", "start"}, "Start screen saver.");
        QCommandLineOption optionConfig({"c", "config"}, "Start config dialog of screen saver.", "screensaer-name", "");

        parser.addOption(optionDbus);
        parser.addOption(optionStart);
        parser.addOption(optionConfig);
        parser.addPositionalArgument("screensaer-name", "Use the screensaver application.", "[name]");
        parser.addHelpOption();
        parser.addVersionOption();

        parser.process(app);
        doStart = !parser.isSet(optionDbus);

        isCustomConfig = parser.isSet(optionConfig);
        configName = parser.value(optionConfig);
    }

    // custom config dialog
    if (isCustomConfig) {
        if (configName.isEmpty()) {
            parser.showHelp(1);
            return 1;
        } else {
            app.setQuitOnLastWindowClosed(true);
            CustomConfig conf;
            if (conf.startCustomConfig(configName))
                return app.exec();
            else
                return 0;
        }
    }

    // 注册DBus服务
    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.\n"
                 "Please check your system settings and try again.\n");

        if (!doStart)
            return -1;
    }

    const QString serviceName = "com.deepin.ScreenSaver";
    const QString objDbusPath = "/com/deepin/ScreenSaver";
    // add our D-Bus interface and connect to D-Bus
    if (!QDBusConnection::sessionBus().registerService(serviceName)) {
        qWarning() << QString("Cannot register the \"%1\" service.\n").arg(serviceName);

        if (!doStart)
            return -1;
    }

    auto server = new DBusScreenSaver();
    Q_UNUSED(new ScreenSaverAdaptor(qobject_cast<DBusScreenSaver*>(server)))

    if (!QDBusConnection::sessionBus().registerObject(objDbusPath, server)) {
        qWarning() << QString("Cannot register to the D-Bus object: \"%1\"\n").arg(objDbusPath);

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
