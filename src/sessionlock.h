// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt 原生窗口的 session-lock 适配；平台私有接口仅在实现中使用。
// 已验证 Qt 5.15 / 6.8；Qt >= 6.9 当前明确失败，不降级为普通全屏窗口。
#pragma once

#include <QObject>

class QWindow;
class SessionLockPrivate;

class SessionLock : public QObject
{
    Q_OBJECT
public:
    explicit SessionLock(QObject *parent = nullptr);
    ~SessionLock() override;
    bool isAvailable() const;
    bool isLocked() const;
    bool isBusy() const;
    void start();
    void stop();
    bool addWindow(QWindow *window);

signals:
    void ready(); // The lock proxy exists; create windows before waiting for locked().
    void locked();
    void stopped();
    void failed(const QString &reason);

private:
    SessionLockPrivate *d;
};
