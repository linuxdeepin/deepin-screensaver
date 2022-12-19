// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "timeintervalwidget.h"

#include <QHBoxLayout>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

#define SPECIFYLINEEDITLENGTH 65

static const int kMaxIntervalTime = 3600;   // second
static const int kMinIntervalTime = 3;   // second
static const int kDefaultTime = 10;

TimeIntervalWidget::TimeIntervalWidget()
{
}

void TimeIntervalWidget::setOption(Dtk::Core::DSettingsOption *option)
{
    if (m_option || !option)
        return;
    m_option = option;

    m_lineEdit = new DLineEdit(this);
    m_lineEdit->setText(option->value().toString());
    m_lineEdit->setClearButtonEnabled(false);
    m_lineEdit->setFixedWidth(SPECIFYLINEEDITLENGTH);
    m_prefixTips = new DLabel(tr("Time interval(s)"), this);
    m_suffixTips = new DLabel(QString("%1: %2-%3")
                                      .arg(tr("Range"))
                                      .arg(QString::number(kMinIntervalTime))
                                      .arg(QString::number(kMaxIntervalTime)),
                              this);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_prefixTips);
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_suffixTips);
    layout->addStretch(3);

    connect(m_lineEdit, &DLineEdit::editingFinished, this, &TimeIntervalWidget::onEditingFinished);
    connect(option, &DSettingsOption::valueChanged, this, &TimeIntervalWidget::onValueChanged);
}

void TimeIntervalWidget::onEditingFinished()
{
    auto intervalTime = m_lineEdit->text().toInt();
    if (intervalTime < kMinIntervalTime || intervalTime > kMaxIntervalTime) {
        intervalTime = kDefaultTime;
        m_lineEdit->setText(QString::number(intervalTime));
    }

    m_option->setValue(QString::number(intervalTime));
    m_lineEdit->clearFocus();
}

void TimeIntervalWidget::onValueChanged(QVariant value)
{
    m_lineEdit->setText(value.toString());
}
