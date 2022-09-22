// SPDX-FileCopyrightText: 2017 ~ 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBUSCUSTOMCONFIG_H
#define DBUSCUSTOMCONFIG_H

#include <QObject>
#include <QTimer>

class ScreenSaverSettingDialog;
class DBusCustomConfig : public QObject
{
    Q_OBJECT
public:
    explicit DBusCustomConfig(QObject *parent = nullptr);
    ~DBusCustomConfig();

    Q_INVOKABLE void StartCustomConfig(const QString &name);
    Q_INVOKABLE QStringList ListWithCustomConfigurations();
    Q_INVOKABLE bool HasConfigSettings(const QString &name);

private:
    ScreenSaverSettingDialog *m_settingDialiog {nullptr};
    QTimer m_autoQuitTimer;
};

#endif // DBUSCUSTOMCONFIG_H
