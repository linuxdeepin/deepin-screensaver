// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SELECTPATHWIDGET_H
#define SELECTPATHWIDGET_H

#include <DLabel>
#include <DPushButton>
#include <DWidget>
#include <DSettingsOption>
#include <DLineEdit>

#include <QScopedPointer>

class TruncateLineEdit;

class SelectPathWidget : public Dtk::Widget::DWidget
{
    Q_OBJECT
public:
    explicit SelectPathWidget(QWidget *parent = nullptr);
    QString getPath();
signals:
    void requsetSetPath();

private:
    void initOption();
private:
    Dtk::Widget::DLabel *m_selectTips = nullptr;
    Dtk::Widget::DLineEdit *m_selectLineEdit = nullptr;
    Dtk::Widget::DPushButton *m_selectBtn = nullptr;
    QString m_url;
};

#endif // SELECTPATHWIDGET_H
