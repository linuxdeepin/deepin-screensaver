// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef XPROXYWINDOW_H
#define XPROXYWINDOW_H

#include <QAbstractNativeEventFilter>
#include <QWidget>

class XProxyWindow : public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit XProxyWindow(WId parent);
    bool init();
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

protected:
    void closeEvent(QCloseEvent *event) override;
    QSize mapFromHandle(const QSize &handleSize);
signals:

public slots:
private:
    WId m_parentId;
};

#endif   // XPROXYWINDOW_H
