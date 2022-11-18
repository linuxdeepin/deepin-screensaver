// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleapplication.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QTranslator>
#include <QFile>
#include <QDir>
#include <QProcess>

SingleApplication::SingleApplication(int &argc, char **argv, int)
    : DApplication(argc, argv),
      m_localServer(new QLocalServer),
      m_commandParser(new QCommandLineParser)
{
    initOptions();
    initConnect();
    userServerName();
}

SingleApplication::~SingleApplication()
{
    if (m_conf) {
        delete m_conf;
        m_conf = nullptr;
    }
    closeServer();
}

void SingleApplication::process(const QStringList &arguments)
{
    m_commandParser->process(arguments);
}

void SingleApplication::initConnect()
{
    connect(m_localServer, &QLocalServer::newConnection, this, &SingleApplication::handleConnection);
}

QLocalSocket *SingleApplication::getNewClientConnect(const QByteArray &message)
{
    QLocalSocket *localSocket = new QLocalSocket;
    localSocket->connectToServer(m_userKey);
    if (localSocket->waitForConnected(1000)) {
        if (localSocket->state() == QLocalSocket::ConnectedState) {
            if (localSocket->isValid()) {
                localSocket->write(message);
                localSocket->flush();
            }
        }
    } else {
        qDebug() << localSocket->errorString();
    }

    return localSocket;
}

void SingleApplication::userServerName()
{
    m_userKey = QString("%1/custom_config").arg(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation));
    if (m_userKey.isEmpty()) {
        m_userKey = QString("%1/custom_config").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    }
}

void SingleApplication::initOptions()
{
    QCommandLineOption optionDbus({"d", "dbus"}, "Register DBus service.");
    QCommandLineOption optionStart({"s", "start"}, "Start screen saver.");
    QCommandLineOption optionConfig({"c", "config"}, "Start config dialog of screen saver.", "screensaer-name", "");

    m_commandParser->addOption(optionDbus);
    m_commandParser->addOption(optionStart);
    m_commandParser->addOption(optionConfig);
    m_commandParser->addPositionalArgument("screensaer-name", "Use the screensaver application.", "[name]");
    m_commandParser->addHelpOption();
    m_commandParser->addVersionOption();
}

void SingleApplication::handleNewClient()
{
    QByteArray data { nullptr };
    bool isSetGetMonitorFiles = false;

    qDebug() << "new client send data: " << arguments();
    for (const QString &arg : arguments()) {

        if (!arg.startsWith("-") && QFile::exists(arg)) {
            data.append(QDir(arg).absolutePath().toLocal8Bit().toBase64());
        }
        else {
            data.append(arg.toLocal8Bit().toBase64());
        }

        data.append(' ');
    }

    if (!data.isEmpty())
        data.chop(1);

    QLocalSocket *socket = SingleApplication::getNewClientConnect(data);
    if (isSetGetMonitorFiles && socket->error() == QLocalSocket::UnknownSocketError) {
        socket->waitForReadyRead();

        for (const QByteArray &i : socket->readAll().split(' '))
            qDebug() << QString::fromLocal8Bit(QByteArray::fromBase64(i));
    }
}

bool SingleApplication::isSet(const QStringList &namelst) const
{
    if (namelst.isEmpty())
        return false;

    for (const QString &name : namelst) {
        if (m_commandParser->isSet(name))
            return true;
    }

    return false;
}

const QString SingleApplication::getOptionValue(const QString &name)
{
    return m_commandParser->value(name);
}

bool SingleApplication::isSingleConfig()
{
    QLocalSocket localSocket;
    localSocket.connectToServer(m_userKey);

    // if connect success, another instance is running.
    if (localSocket.waitForConnected(1000))
        return false;

    m_localServer->removeServer(m_userKey);
    bool f = m_localServer->listen(m_userKey);
    return f;
}

QStringList SingleApplication::positionalArguments()
{
    return m_commandParser->positionalArguments();
}

bool SingleApplication::startCustomConfig()
{
    QCommandLineOption optionConfig({"c", "config"}, "Start config dialog of screen saver.", "screensaer-name", "");
    QString configName = m_commandParser->value(optionConfig);
    if (configName.isEmpty())
        return false;

    if (this->isSingleConfig()) {
        if (!m_conf)
            m_conf = new CustomConfig();
        return m_conf->startCustomConfig(configName);
    } else {
        this->handleNewClient();
        return false;
    }
}

void SingleApplication::handleConnection()
{
    qDebug() << "new connection is coming";
    QLocalSocket *nextPendingConnection = m_localServer->nextPendingConnection();
    connect(nextPendingConnection, SIGNAL(readyRead()), this, SLOT(readData()));
}

void SingleApplication::readData()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());

    if (!socket)
        return;

    QStringList arguments;
    for (const QByteArray &arg_base64 : socket->read(2048).split(' ')) {
        const QByteArray &arg = QByteArray::fromBase64(arg_base64.simplified());
        if (arg.isEmpty())
            continue;
        arguments << QString::fromLocal8Bit(arg);
    }

    m_commandParser->process(arguments);

    qDebug() << "m_userKey: " << m_userKey;
    qDebug() << "arguments: " << arguments;
    QCommandLineOption optionConfig({"c", "config"}, "Start config dialog of screen saver.", "screensaer-name", "");
    if (m_commandParser->isSet(optionConfig) && m_conf) {
        QString configName = m_commandParser->value(optionConfig);
        if (!configName.isEmpty())
            m_conf->startCustomConfig(configName);
    }
}

void SingleApplication::closeServer()
{
    if (m_localServer) {
        m_localServer->removeServer(m_localServer->serverName());
        m_localServer->close();
        delete m_localServer;
        m_localServer = nullptr;
    }
}
