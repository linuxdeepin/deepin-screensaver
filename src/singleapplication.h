// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include "customconfig.h"

#include <DApplication>
#include <QCommandLineParser>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
QT_END_NAMESPACE

class SingleApplication : public DTK_WIDGET_NAMESPACE::DApplication
{
    Q_OBJECT
public:
    explicit SingleApplication(int &argc, char **argv, int = ApplicationFlags);
    ~SingleApplication() override;

    void process(const QStringList &arguments);
    void handleNewClient();
    bool isSet(const QStringList &namelst) const;
    const QString getOptionValue(const QString &name);
    bool isSingleConfig();
    QStringList positionalArguments();
    bool startCustomConfig();
public slots:
    void handleConnection();
    void readData();
    void closeServer();

private:
    QLocalSocket *getNewClientConnect(const QByteArray &message);
    void userServerName();
    void initOptions();
    void initConnect();

private:
    QLocalServer *m_localServer { nullptr };
    QCommandLineParser *m_commandParser { nullptr };
    CustomConfig *m_conf { nullptr };
    QString m_userKey;
};

#endif // SINGLEAPPLICATION_H
