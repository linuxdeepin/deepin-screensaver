// SPDX-FileCopyrightText: 2017 ~ 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "selectpathwidget.h"
#include "truncatelineedit.h"

#include <DFileDialog>

#include <QHBoxLayout>
#include <QEvent>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

SelectPathWidget::SelectPathWidget(QWidget *parent)
    : DWidget(parent)
{

}

void SelectPathWidget::setOption(Dtk::Core::DSettingsOption *option)
{
    if (m_option || !option)
        return;

    m_option = option;

    QString valueText = option->value().toString();
    // 路径转换
    if (valueText.startsWith(QStringLiteral("~")))
        valueText.replace(QStringLiteral("~"), QDir::homePath());

    m_selectTips = new DLabel(tr("Picture path"), this);
    m_selectLineEdit = new TruncateLineEdit(this);
    m_selectBtn = new DPushButton(QStringLiteral("..."), this);

    m_selectLineEdit->setElideMode(Qt::ElideMiddle);
    m_selectLineEdit->setText(valueText);
    m_selectLineEdit->setPlaceholderText(tr("Select a path of pictures"));
    m_selectLineEdit->setClearButtonEnabled(false);
    m_selectLineEdit->lineEdit()->installEventFilter(this);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_selectTips);
    layout->addWidget(m_selectLineEdit);
    layout->addWidget(m_selectBtn);

    connect(m_selectBtn, &DPushButton::clicked, option, [=]() {
        QString currentPath = m_selectLineEdit->getText();

        // 如果未选择过，或选择的目录已经不存在，则重置为默认值
        if (currentPath.isEmpty() || !QFile::exists(currentPath)) {
            currentPath = option->defaultValue().toString();
            if (currentPath.startsWith(QStringLiteral("~")))
                currentPath.replace(QStringLiteral("~"), QDir::homePath());

            // 如果默认值为空或不存在，则强制设置为图片目录
            if (currentPath.isEmpty() || !QFile::exists(currentPath)) {
                currentPath = QDir::homePath() + "/Pictures";
                qWarning() << "The default is empty or does not exist,set path is " << currentPath;
            }
        }

        QString selectPath = DFileDialog::getExistingDirectory(nullptr, tr("Picture path"), currentPath,
                                                               QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (selectPath.isEmpty())
            return ;

        option->setValue(selectPath);
    });

    connect(option, &DSettingsOption::valueChanged, this, [=](QVariant value) {
        QString currentPath = value.toString();
        if (currentPath.startsWith(QStringLiteral("~"))) {
            // 如果配置的默认值是~，则恢复默认将触发该逻辑，此时纠正为homePath进行保存
            currentPath.replace(QStringLiteral("~"), QDir::homePath());
            option->setValue(currentPath);
            return ;
        }
        m_selectLineEdit->setText(currentPath);
    });
}

bool SelectPathWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (m_selectLineEdit && obj == m_selectLineEdit->lineEdit()) {
        if (event->type() == QEvent::MouseButtonPress
                || event->type() == QEvent::MouseButtonRelease
                || event->type() == QEvent::MouseButtonDblClick
                || event->type() == QEvent::MouseMove )
            return true;
    }

    return QObject::eventFilter(obj, event);
}
