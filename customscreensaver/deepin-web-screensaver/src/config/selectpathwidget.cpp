// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "selectpathwidget.h"
#include "../slideshowconfig.h"

#include <DFileDialog>

#include <QHBoxLayout>
#include <QEvent>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

SelectPathWidget::SelectPathWidget(QWidget *parent)
    : DWidget(parent)
{
    initOption();
}

QString SelectPathWidget::getPath()
{
    return m_selectLineEdit->text();
}

void SelectPathWidget::initOption()
{
    QString valueText;
    // 路径转换
    if (valueText.startsWith(QStringLiteral("~")))
        valueText.replace(QStringLiteral("~"), QDir::homePath());

    m_selectTips = new DLabel(tr("Enter web url"), this);
    m_selectLineEdit = new DLineEdit(this);
    m_selectBtn = new DPushButton(tr("apply"), this);
    m_selectBtn->setFocusPolicy(Qt::NoFocus);

    m_url = SlideshowConfig::instance()->slideshowPath();
    m_selectLineEdit->setPlaceholderText(m_url);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_selectTips);
    layout->addWidget(m_selectLineEdit);
    layout->addWidget(m_selectBtn);

    connect(m_selectBtn, &DPushButton::clicked, this, &SelectPathWidget::requsetSetPath);
}
