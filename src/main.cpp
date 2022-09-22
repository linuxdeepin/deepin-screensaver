// SPDX-FileCopyrightText: 2017 ~ 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <DApplication>

#include <QCommandLineParser>
#include <QCommandLineOption>

#include "screensaver_adaptor.h"
#include "customconfig_adaptor.h"
#include "dbusscreensaver.h"
#include "dbuscustomconfig.h"


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
    bool isConfigService = false;

    QString serviceName = "com.deepin.ScreenSaver";
    QString objDbusPath = "/com/deepin/ScreenSaver";
    QCommandLineParser parser;

    if (argc > 1) {
        QCommandLineOption optionDbus({"d", "dbus"}, "Register DBus service.");
        QCommandLineOption optionStart({"s", "start"}, "Start screen saver.");
        QCommandLineOption optionConfig({"c", "config"}, "Start config dialog of screen saver.");

        parser.addOption(optionDbus);
        parser.addOption(optionStart);
        parser.addOption(optionConfig);
        parser.addPositionalArgument("screensaer-name", "Use the screensaver application.", "[name]");
        parser.addHelpOption();
        parser.addVersionOption();

        parser.process(app);
        doStart = !parser.isSet(optionDbus);

        if (!parser.isSet(optionStart) && parser.isSet(optionConfig)) {
            isConfigService = true;
            serviceName = "org.deepin.screensaver.CustomConfig";
            objDbusPath = "/org/deepin/screensaver/CustomConfig";
        }
    }

    // 注册DBus服务
    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.\n"
                 "Please check your system settings and try again.\n");

        if (!doStart)
            return -1;
    }

    // add our D-Bus interface and connect to D-Bus
    if (!QDBusConnection::sessionBus().registerService(serviceName)) {
        qWarning() << QString("Cannot register the \"%1\" service.\n").arg(serviceName);

        if (!doStart)
            return -1;
    }

    QObject *service = nullptr;
    if (isConfigService) {
        service = new DBusCustomConfig();
        Q_UNUSED(new CustomConfigAdaptor(qobject_cast<DBusCustomConfig*>(service)))
    }
    else {
        service = new DBusScreenSaver();
        Q_UNUSED(new ScreenSaverAdaptor(qobject_cast<DBusScreenSaver*>(service)))
    }

    if (!QDBusConnection::sessionBus().registerObject(objDbusPath, service)) {
        qWarning() << QString("Cannot register to the D-Bus object: \"%1\"\n").arg(objDbusPath);

        if (!doStart)
            return -1;
    }

    if (doStart) {
        if (isConfigService) {
            DBusCustomConfig *screensaverService = qobject_cast<DBusCustomConfig*>(service);
            screensaverService->StartCustomConfig(parser.positionalArguments().value(0));
        } else {
            DBusScreenSaver *screensaverService = qobject_cast<DBusScreenSaver*>(service);
            if (argc > 1)
                screensaverService->Start(parser.positionalArguments().value(0));
            else
                screensaverService->Start();
        }
    }

    return app.exec();
}
