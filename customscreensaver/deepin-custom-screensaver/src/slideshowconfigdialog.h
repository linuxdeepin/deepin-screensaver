// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SLIDESHOWCONFIGDIALOG_H
#define SLIDESHOWCONFIGDIALOG_H

#include <DApplication>
#include <DDialog>
#include <DLineEdit>
#include <DCheckBox>

DWIDGET_USE_NAMESPACE

class SlideShowConfigDialog : public DDialog
{
    Q_OBJECT

public:
    explicit SlideShowConfigDialog(QWidget *parent = nullptr);
    ~SlideShowConfigDialog();

    void startConfig();
protected:
    void mousePressEvent(QMouseEvent *) override;

private slots:
    void setSlideShowPath();
    void setIntervalTime();
    void setPlayMode(const bool &mode);

private:

    void initUI();
    void initConnect();
    void initUiValue();
    QString defaultPath();

    DLineEdit *m_slideShowPathEdit;
    DPushButton *m_openFileBtn;
    DLineEdit *m_intervalTimeEdit;
    DCheckBox *m_enableShuffleBox;

    bool m_enableShuffle = false;
    QString m_slideShowPath;
};

#endif // SLIDESHOWCONFIGDIALOG_H
