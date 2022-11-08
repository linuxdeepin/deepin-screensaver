// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleapplication.h"

#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>

SingleApplication::SingleApplication()
{
    m_localServer = new QLocalServer;
}

SingleApplication::~SingleApplication()
{
    if (m_localServer){
        m_localServer->removeServer(m_localServer->serverName());
        m_localServer->close();
        delete m_localServer;
        m_localServer = nullptr;
    }
}

QString SingleApplication::userServerName(const QString &key)
{
    QString userKey = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation), key);
    if (userKey.isEmpty()) {
        userKey = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation), key);
    }
    return userKey;
}

bool SingleApplication::setSingleInstance(const QString &key)
{
    QString userKey = userServerName(key);

    QLocalSocket localSocket;
    localSocket.connectToServer(userKey);

    // if connect success, another instance is running.
    bool result = localSocket.waitForConnected(1000);

    if (result)
        return false;

    m_localServer->removeServer(userKey);

    bool f = m_localServer->listen(userKey);
    if (f) {
        if(m_localServer->serverError()== QAbstractSocket::AddressInUseError
                && QFile::exists(m_localServer->serverName())){
            QFile::remove(m_localServer->serverName());
            m_localServer->listen(userKey);
        }
    }

    return f;
}
