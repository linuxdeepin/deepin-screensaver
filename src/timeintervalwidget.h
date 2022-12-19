// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TIMEINTERVALWIDGET_H
#define TIMEINTERVALWIDGET_H

#include <DWidget>
#include <DLabel>
#include <DLineEdit>
#include <DSettingsOption>

class TimeIntervalWidget : public Dtk::Widget::DWidget
{
public:
    TimeIntervalWidget();
    void setOption(Dtk::Core::DSettingsOption *option);

private Q_SLOTS:
    void onEditingFinished();
    void onValueChanged(QVariant value);

private:
    Dtk::Widget::DLabel *m_prefixTips = nullptr;
    Dtk::Widget::DLabel *m_suffixTips = nullptr;
    Dtk::Widget::DLineEdit *m_lineEdit = nullptr;
    Dtk::Core::DSettingsOption *m_option = nullptr;
};

#endif   // TIMEINTERVALWIDGET_H
