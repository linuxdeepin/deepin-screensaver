// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SLIDESHOWCONFIGDIALOG_H
#define SLIDESHOWCONFIGDIALOG_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <DConfig>
#include <QCheckBox>
#include <QScopedPointer>

DCORE_USE_NAMESPACE

class SlideShowConfigDialog : public QWidget
{
    Q_OBJECT

public:
    SlideShowConfigDialog(QWidget *parent = nullptr);
    ~SlideShowConfigDialog();

    bool dconfigValid();
    void startConfig();

protected:
    void mousePressEvent(QMouseEvent *) override;

private slots:
    void setSlideShowPath();
    void setIntervalTime();
    void setShuffle(const bool shuffle);

private:
    void initUI();

    QLineEdit *m_slideShowPathEdit;
    QLineEdit *m_intervalTimeEdit;
    QCheckBox *m_enableShuffleBox;

    bool m_enableShuffle = false;
    QString m_slideShowPath;

    DConfig *m_dCfg;
};

#endif // SLIDESHOWCONFIGDIALOG_H
