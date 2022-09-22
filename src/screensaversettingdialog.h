// SPDX-FileCopyrightText: 2017 ~ 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCREENSAVERSETTINGDIALOG_H
#define SCREENSAVERSETTINGDIALOG_H

#include <DSettingsDialog>

#include <QScopedPointer>

DCORE_BEGIN_NAMESPACE
class QSettingBackend;
class DSettings;
DCORE_END_NAMESPACE

class SelectPathWidget;

class ScreenSaverSettingDialog : public Dtk::Widget::DSettingsDialog
{
    Q_OBJECT
public:
    explicit ScreenSaverSettingDialog(const QString &name, QWidget *parent = nullptr);
    ~ScreenSaverSettingDialog();

public:
    static QPair<QWidget*, QWidget*> createSelectPathWidget(QObject *obj);

public:
    static SelectPathWidget *m_selectPathWidget;

private:
    QScopedPointer<Dtk::Core::QSettingBackend> m_backend;
    QScopedPointer<Dtk::Core::DSettings> m_settings;

    QString m_screenSaverName;
};

#endif // SCREENSAVERSETTINGDIALOG_H
