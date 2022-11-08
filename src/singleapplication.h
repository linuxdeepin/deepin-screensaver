// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include <QtGlobal>

#include <DApplication>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
QT_END_NAMESPACE

DWIDGET_USE_NAMESPACE

class SingleApplication : public QObject
{
    Q_OBJECT
public:
    explicit SingleApplication();
    ~SingleApplication();

    static QString userServerName(const QString &key);

public:
    bool setSingleInstance(const QString &key);

private:
    QLocalServer *m_localServer;
};

#endif // SINGLEAPPLICATION_H
