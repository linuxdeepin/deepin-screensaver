// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SLIDESHOWCONFIGDIALOG_H
#define SLIDESHOWCONFIGDIALOG_H

#include "config/selectpathwidget.h"
#include "singleinstance.h"

#include <DAbstractDialog>
#include <DCheckBox>
#include <DTitlebar>

class SlideShowConfigDialog : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT
public:
    explicit SlideShowConfigDialog(QWidget *parent = nullptr);
    ~SlideShowConfigDialog();
    void startConfig();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void handArguments(const QStringList &);
private slots:
    void selectPath();

private:
    void initUI();
    void initConnect();

private:
    DTK_WIDGET_NAMESPACE::DTitlebar *frameBar = nullptr;
    SelectPathWidget *pathWidget = nullptr;
    Dtk::Widget::DPushButton *m_confirmBtn = nullptr;
};

#endif // SLIDESHOWCONFIGDIALOG_H
